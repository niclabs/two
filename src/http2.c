#include "http2.h"
#include "logging.h"
#include "utils.h"


/*
* This API assumes the existence of a TCP connection between Server and Client,
* and two methods to use this TCP connection that are http_read and http_write
*/

/*--------------Internal methods-----------------------*/

/*
* Function: init_variables
* Initialize default variables of the current http2 connection
* Input: -> st: pointer to a hstates_t struct where variables of the connection
*           are going to be stored.
* Output: 0 if initialize were made. -1 if not.
*/
int init_variables(hstates_t * st){
    st->h2s.remote_settings[0] = st->h2s.local_settings[0] = DEFAULT_HTS;
    st->h2s.remote_settings[1] = st->h2s.local_settings[1] = DEFAULT_EP;
    st->h2s.remote_settings[2] = st->h2s.local_settings[2] = DEFAULT_MCS;
    st->h2s.remote_settings[3] = st->h2s.local_settings[3] = DEFAULT_IWS;
    st->h2s.remote_settings[4] = st->h2s.local_settings[4] = DEFAULT_MFS;
    st->h2s.remote_settings[5] = st->h2s.local_settings[5] = DEFAULT_MHLS;
    st->h2s.wait_setting_ack = 0;
    memset(&st->h2s.current_stream, 0, sizeof(h2_stream_t));
    return 0;
}

/*
* Function: update_remote_settings
* Updates the table where remote settings are stored
* Input: -> sframe: it must be a SETTINGS frame
*        -> place: must be LOCAL or REMOTE. It indicates which table to update.
         -> st: pointer to hstates_t struct where settings table are stored.
* Output: 0 if update was successfull, -1 if not
*/
int update_settings_table(settings_payload_t *spl, uint8_t place, hstates_t *st){
    /*spl is for setttings payload*/
    uint8_t i;
    uint16_t id;
    int rc = 0;
    /*Verify the values of settings*/
    for(i = 0; i < spl->count; i++){
        rc += verify_setting(spl->pairs[i].identifier, spl->pairs[i].value);
    }
    if(rc != 0){
        puts("Error: invalid setting found");
        return -1;
    }
    if(place == REMOTE){
        /*Update remote table*/
        for(i = 0; i < spl->count; i++){
            id = spl->pairs[i].identifier;
            st->h2s.remote_settings[--id] = spl->pairs[i].value;
        }
        return 0;
    }
    else if(place == LOCAL){
        /*Update local table*/
        for(i = 0; i < spl->count; i++){
            id = spl->pairs[i].identifier;
            st->h2s.local_settings[--id] = spl->pairs[i].value;
        }
        return 0;
    }
    else{
        puts("Error: Not a valid table to update");
        return -1;
    }
}

/*
* Function: send_settings_ack
* Sends an ACK settings frame to endpoint
* Input: -> st: pointer to hstates struct where http and http2 connection info is
* stored
* Output: 0 if sent was successfully made, -1 if not.
*/
int send_settings_ack(hstates_t * st){
    frame_t ack_frame;
    frame_header_t ack_frame_header;
    int rc;
    rc = create_settings_ack_frame(&ack_frame, &ack_frame_header);
    if(rc){
        puts("Error in Settings ACK creation!");
        return -1;
    }
    uint8_t byte_ack[9+0]; /*Settings ACK frame only has a header*/
    int size_byte_ack = frame_to_bytes(&ack_frame, byte_ack);
    rc = http_write(byte_ack, size_byte_ack, st);
    if(rc != size_byte_ack){
        puts("Error in Settings ACK sending");
        return -1;
    }
    return 0;
}

/*
* Function: check_for_settings_ack
* Verifies the correctness of header and checks if frame settings is an ACK.
* Input: -> header: settings frame's header to read
*        -> st: pointer to hstates struct where connection variables are stored
* Output: 0 if ACK was not setted. 1 if it was. -1 if error was found.
*/
int check_for_settings_ack(frame_header_t *header, hstates_t *st){
    if(header->type != 0x4){
        puts("Read settings payload error, header type is not SETTINGS");
        return -1;
    }
    else if(header->stream_id != 0){
        puts("Protocol Error: stream id on SETTINGS FRAME is not zero");
        return -1;
    }
        /*Check if ACK is set*/
    else if(is_flag_set(header->flags, SETTINGS_ACK_FLAG)){
        if(header->length != 0){
            puts("Frame Size Error: ACK flag is set, but payload size is not zero");
            return -1;
        }
        else{
            if(st->h2s.wait_setting_ack){
                st->h2s.wait_setting_ack = 0;
                return 1;
            }
            else{
                puts("ACK received but not expected");
                return 1;
            }
        }
    }
    else{
        return 0;
    }
    return 0;
}

/*
* Function: read_settings_payload
* Reads a settings payload from buffer and works with it.
* Input: -> buff_read: buffer where payload's data is written
        -> header: pointer to a frameheader_t structure already built with frame info
        -> spl: pointer to settings_payload_t struct where data is gonna be written
        -> pairs: pointer to settings_pair_t array where data is gonna be written
        -> st: pointer to hstates_t struct where connection variables are stored
* Output: 0 if operations are done successfully, -1 if not.
*/
int read_settings_payload(uint8_t *buff_read, frame_header_t *header, settings_payload_t *spl, settings_pair_t *pairs, hstates_t *st){
    int size = bytes_to_settings_payload(buff_read, header->length, spl, pairs);
    if(size != header->length){
        puts("Error in byte to settings payload coding");
        return -1;
    }
    if(!update_settings_table(spl, REMOTE, st)){
        send_settings_ack(st);
        return 0;
    }
    else{
        /*TODO: send protocol error*/
        return -1;
    }
}

/*
* Function: read_frame
* Build a header and writes the header's payload's bytes on buffer.
* Input: -> buff_read: buffer where payload bytes are going to be stored.
        -> header: pointer to the frameheader_t where header is going to be stored
        -> st: pointer to hstates_t struct where variables are stored
* Output: 0 if writing and building is done successfully. -1 if not.
*/
int read_frame(uint8_t *buff_read, frame_header_t *header, hstates_t *st){
    int rc = read_n_bytes(buff_read, 9, st);
    if(rc != 9){
        puts("Error reading bytes from http");
        return -1;
    }
    /*Must be 0*/
    rc = bytes_to_frame_header(buff_read, 9, header);
    if(rc){
        puts("Error coding bytes to frame header");
        return -1;
    }
    if(header->length > 256){
        printf("Error: Payload's size (%u) too big (>256)\n", header->length);
        return -1;
    }
    rc = read_n_bytes(buff_read, header->length, st);
    if(rc != header->length){
        puts("Error reading bytes from http");
        return -1;
    }
    return 0;
}

/*
* Function: check_headers_stream_condition
* Checks the stream conditionals and return a value depending on the current
* stream state and id, and the stream_id given in the header of the frame
* Input: -> st: hstates_t struct where stream variables are stored
*         -> header: header of the incoming frame
* Ouput:
*/
int check_headers_stream_condition(hstates_t *st, frame_header_t *header){
  (void) st;
  (void) header;
  return 0;
}

/*----------------------API methods-------------------*/

/*
* Function: h2_send_local_settings
* Sends local settings to endpoint.
* Input: -> st: pointer to hstates_t struct where local settings are stored
* Output: 0 if settings were sent. -1 if not.
*/
int h2_send_local_settings(hstates_t *st){
    int rc;
    uint16_t ids[6] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6};
    frame_t mysettingframe;
    frame_header_t mysettingframeheader;
    settings_payload_t mysettings;
    settings_pair_t mypairs[6];
    /*rc must be 0*/
    rc = create_settings_frame(ids, st->h2s.local_settings, 6, &mysettingframe,
                               &mysettingframeheader, &mysettings, mypairs);
    if(rc){
        puts("Error in Settings Frame creation");
        return -1;
    }
    uint8_t byte_mysettings[9+6*6]; /*header: 9 bytes + 6 * setting: 6 bytes */
    int size_byte_mysettings = frame_to_bytes(&mysettingframe, byte_mysettings);
    /*Assuming that http_write returns the number of bytes written*/
    rc = http_write(byte_mysettings, size_byte_mysettings, st);
    if(rc != size_byte_mysettings){
        puts("Error in local settings writing");
        return -1;
    }
    /*Settings were sent, so we expect an ack*/
    st->h2s.wait_setting_ack = 1;
    return 0;
}

/*
* Function: h2_read_setting_from
* Reads a setting parameter from local or remote table
* Input: -> place: must be LOCAL or REMOTE. It indicates the table to read.
*        -> param: it indicates which parameter to read from table.
*        -> st: pointer to hstates_t struct where settings tables are stored.
* Output: The value read from the table. -1 if nothing was read.
*/

uint32_t h2_read_setting_from(uint8_t place, uint8_t param, hstates_t *st){
    if(param < 1 || param > 6){
        printf("Error: %u is not a valid setting parameter\n", param);
        return -1;
    }
    else if(place == LOCAL){
        return st->h2s.local_settings[--param];
    }
    else if(place == REMOTE){
        return st->h2s.remote_settings[--param];
    }
    else{
        puts("Error: not a valid table to read from");
        return -1;
    }
    return -1;
}

/*
* Function: h2_client_init_connection
* Initializes HTTP2 connection between endpoints. Sends preface and local
* settings.
* Input: -> st: pointer to hstates_t struct where variables of client are going
*               to be stored.
* Output: 0 if connection was made successfully. -1 if not.
*/
int h2_client_init_connection(hstates_t *st){
    int rc = init_variables(st);
    char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    uint8_t preface_buff[24];
    puts("Client: sending preface...");
    uint8_t i = 0;
    /*We load the buffer with the ascii characters*/
    while(preface[i] != '\0'){
        preface_buff[i] = preface[i];
        i++;
    }
    rc = http_write(preface_buff,24, st);
    if(rc != 24){
        puts("Error in preface sending");
        return -1;
    }
    puts("Client: sending local settings...");
    if((rc = h2_send_local_settings(st)) < 0){
        puts("Error in local settings sending");
        return -1;
    }
    puts("Client: init connection done");
    return 0;
}

/*
* Function: h2_server_init_connection
* Initializes HTTP2 connection between endpoints. Waits for preface and sends
* local settings.
* Input: -> st: pointer to hstates_t struct where variables of client are going
*               to be stored.
* Output: 0 if connection was made successfully. -1 if not.
*/
int h2_server_init_connection(hstates_t *st){
    int rc = init_variables(st);
    char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    uint8_t preface_buff[25];
    preface_buff[24] = '\0';
    puts("Server: waiting for preface...");
    /*We read the first 24 byes*/
    rc = read_n_bytes(preface_buff, 24, st);
    if(rc != 24){
        puts("Error in reading preface");
        return -1;
    }
    puts("Server: 24 bytes read");
    if(strcmp(preface, (char*)preface_buff) != 0){
        puts("Error in preface receiving");
        return -1;
    }
    /*Server sends local settings to endpoint*/
    puts("Server: sending local settings...");
    if((rc = h2_send_local_settings(st)) < 0){
        puts("Error in local settings sending");
        return -1;
    }
    puts("Server: init connection done");
    return 0;
}






/*
* Function: h2_receive_frame
* Receives a frame from endpoint, decodes it and works with it.
* Input: void
* Output: 0 if no problem was found. -1 if error was found.
*/
int h2_receive_frame(hstates_t *st){
    uint8_t buff_read[MAX_BUFFER_SIZE];
    //uint8_t buff_write[MAX_BUFFER_SIZE]
    int rc;
    frame_header_t header;
    rc = read_frame(buff_read, &header, st);
    if(rc == -1){
        puts("Error reading frame");
        return -1;
    }
    switch(header.type){
        case DATA_TYPE://Data
            WARN("TODO: Data Frame. Not implemented yet.");
            return -1;
        case HEADERS_TYPE:{//Header
            // Check if stream is not created or previous one is closed
            if(st->h2s.current_stream.stream_id == 0 ||
                (st->h2s.current_stream.state == STREAM_CLOSED &&
                st->h2s.current_stream.stream_id < header.stream_id)){
                //we create a new stream
                st->h2s.current_stream.stream_id = header.stream_id;
                st->h2s.current_stream.state = STREAM_OPEN;
            }
            // Stream id mismatch
            else if(header.stream_id != st->h2s.current_stream.stream_id){
                ERROR("Stream ids do not match. PROTOCOL ERROR.");
                //Send reject error. Write error to HTTP. Check if error is relevant to HTTP or HTTP2
                return -1;
            }
            // Current stream is not open
            else if(st->h2s.current_stream.state != STREAM_OPEN){
              ERROR("Current stream is not open. STREAM CLOSED ERROR");
              return -1;
            }
            headers_payload_t hpl;
            uint8_t headers_block_fragment[64];
            uint8_t padding[32];
            rc = read_headers_payload(buff_read, &header, &hpl, headers_block_fragment, padding);
            if(rc != 0){
                ERROR("Error in headers payload");
                return rc;
            }

            //first we recveive fragments, so we save those on the st->header_block_fragments buffer
            st->waiting_for_end_headers_flag = 1;
            rc = buffer_copy(st->header_block_fragments, hpl.header_block_fragment, get_header_block_fragment_size(&header, &hpl));
            if(rc >= 128){
                ERROR("Header block fragments to big (not enough space allocated).");
                return -1;
            }

            //when receive (continuation or header) frame with flag end_header then the fragments can be decoded, and the headers can be obtained.
            if(is_flag_set(header.flags,HEADERS_END_HEADERS_FLAG)){
                rc = receive_header_block(st->header_block_fragments, st->header_block_fragments_pointer,st->header_list, st->table_count);//return size of header_list (header_count)
                st->waiting_for_end_headers_flag = 0;
            }

            if(is_flag_set(header.flags,HEADERS_END_STREAM_FLAG)){
              st->h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
            }

            /*TODO: Implement read_headers. It uses the hpl to write on header_list.
            * we assume that returns the number of headers pairs written.
            */
            // read_headers(*header_payload_t, *header_list, header_list_start, header_list_max)
            //rc = read_headers(&hpl, st->header_list, st->header_count, HTTP2_MAX_HEADER_COUNT);

            if (rc < 0) {
                ERROR("Error reading headers");
                // TODO: send internal error if number of headers > HTTP2_MAX_HEADER_COUNT
                return rc;
            }
            st->header_count += rc;
        }

            WARN("TODO: Header Frame. Not implemented yet.");
            return -1;
        case PRIORITY_TYPE://Priority
            WARN("TODO: Priority Frame. Not implemented yet.");
            return -1;
        case RST_STREAM_TYPE://RST_STREAM
            WARN("TODO: Reset Stream Frame. Not implemented yet.");
            return -1;
        case SETTINGS_TYPE:{//Settings
            rc = check_for_settings_ack(&header, st);
            if(rc < 0){
                puts("Error was found in SETTINGS Header");
                return -1;
            }
            else if(rc){ /*Frame was an ACK*/
                return 0;
            }
            settings_payload_t spl;
            settings_pair_t pairs[header.length/6];
            rc = read_settings_payload(buff_read, &header, &spl, pairs, st);
            if(rc == -1){
                puts("Error in read settings payload");
                return -1;
            }
            return rc;
        }
        case PUSH_PROMISE_TYPE://Push promise
            WARN("TODO: Push promise frame. Not implemented yet.");
            return -1;
        case PING_TYPE://Ping
            WARN("TODO: Ping frame. Not implemented yet.");
            return -1;
        case GOAWAY_TYPE://Go Avaw
            WARN("TODO: Go away frame. Not implemented yet.");
            return -1;
        case WINDOW_UPDATE_TYPE://Window update
            WARN("TODO: Window update frame. Not implemented yet.");
            return -1;
        case CONTINUATION_TYPE:{//Continuation
            // First verify stream state
            if(header.stream_id == 0x0 ||
              header.stream_id != st->h2s.current_stream.stream_id){
              ERROR("Continuation received on invalid stream. PROTOCOL ERROR");
              return -1;
            }
            else if(st->h2s.current_stream.state != STREAM_OPEN){
              ERROR("Continuation received on closed stream. CLOSED STREAM ERROR");
              return -1;
            }
            if(!st->waiting_for_end_headers_flag){
              ERROR("Continuation must be preceded by a HEADERS frame. PROTOCOL ERROR");
              return -1;
            }
            continuation_payload_t contpl;
            (void) contpl;
            //TODO: write continuation payload and store the block fragment
            if(is_flag_set(header.flags, CONTINUATION_END_HEADERS_FLAG)){
              rc = receive_header_block(st->header_block_fragments, st->header_block_fragments_pointer,st->header_list, st->table_count);//return size of header_list (header_count)
              st->waiting_for_end_headers_flag = 0;
            }
            return 0;

        }
        default:
            WARN("Error: Type not found");
            return -1;
    }
}
