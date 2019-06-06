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
    rc = http_write(st, byte_ack, size_byte_ack);
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
* Function: handle_settings_payload
* Reads a settings payload from buffer and works with it.
* Input: -> buff_read: buffer where payload's data is written
        -> header: pointer to a frameheader_t structure already built with frame info
        -> spl: pointer to settings_payload_t struct where data is gonna be written
        -> pairs: pointer to settings_pair_t array where data is gonna be written
        -> st: pointer to hstates_t struct where connection variables are stored
* Output: 0 if operations are done successfully, -1 if not.
*/
int handle_settings_payload(uint8_t *buff_read, frame_header_t *header, settings_payload_t *spl, settings_pair_t *pairs, hstates_t *st){
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
* Function: check_incoming_headers_condition
* Checks the incoming frame stream_id and the current stream stream_id and
* verifies its correctness. Creates a new stream if needed.
* Input: -> header: header of the incoming headers frame
*        -> st: hstates_t struct where stream variables are stored
* Ouput: 0 if no errors were found, -1 if protocol error was found, -2 if
* stream closed error was found.
*/
int check_incoming_headers_condition(frame_header_t *header, hstates_t *st){
  // Check if stream is not created or previous one is closed
  if(st->h2s.waiting_for_end_headers_flag){
    //protocol error
    ERROR("CONTINUATION frame was expected");
    return -1;
  }
  else if(st->h2s.current_stream.stream_id == 0 ||
      (st->h2s.current_stream.state == STREAM_CLOSED &&
      st->h2s.current_stream.stream_id < header->stream_id)){
      //we create a new stream
      st->h2s.current_stream.stream_id = header->stream_id;
      st->h2s.current_stream.state = STREAM_OPEN;
      return 0;
  }
  // Stream id mismatch
  else if(header->stream_id != st->h2s.current_stream.stream_id){
      //protocol error
      ERROR("Stream ids do not match.");
      return -1;
  }
  // Current stream is not open
  else if(st->h2s.current_stream.state != STREAM_OPEN){
      //stream closed error
      ERROR("Current stream is not open.");
      return -2;
  }
  else{
    return 0;
  }
}

/*
* Function: check_incoming_continuation_condition
* Checks the incoming frame stream_id and the current stream stream_id. Also,
* verify if there was a HEADERS FRAME before the current CONTINUATION FRAME.
* Input : -> header: header of the incoming frame
*         -> st: hstates_t struct where stream variables are stored
* Ouput: 0 if no errors were found, -1 if protocol error was found, -2 if
* stream closed error was found.
*/
int check_incoming_continuation_condition(frame_header_t *header, hstates_t *st){
  // First verify stream state
  if(header->stream_id == 0x0 ||
    header->stream_id != st->h2s.current_stream.stream_id){
    ERROR("Continuation received on invalid stream.");
    return -1;
  }
  else if(st->h2s.current_stream.state != STREAM_OPEN){
    ERROR("Continuation received on closed stream.");
    return -2;
  }
  if(!st->h2s.waiting_for_end_headers_flag){
    ERROR("Continuation must be preceded by a HEADERS frame.");
    return -1;
  }
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
    rc = http_write(st, byte_mysettings, size_byte_mysettings);
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
    rc = http_write(st, preface_buff,24);
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
    uint8_t buff_read[HTTP2_MAX_BUFFER_SIZE];
    //uint8_t buff_write[HTTP2_MAX_BUFFER_SIZE]
    int rc;
    frame_header_t header;
    rc = read_frame(buff_read, &header, st);
    if(rc == -1){
        ERROR("Error reading frame");
        return -1;
    }
    switch(header.type){
        case DATA_TYPE://Data
            WARN("TODO: Data Frame. Not implemented yet.");
            return -1;
        case HEADERS_TYPE:{//Header
            // returns -1 if protocol error was found, -2 if stream closed error, 0 if no errors found
            rc = check_incoming_headers_condition(&header, st);
            if(rc == -1){
              ERROR("PROTOCOL ERROR was found.");
              return -1;
            }
            else if(rc == -2){
              ERROR("STREAM CLOSED ERROR was found");
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
            // we receive a headers, so it could be continuation frames
            st->h2s.waiting_for_end_headers_flag = 1;
            st->keep_receiving = 1;
            int hbf_size = get_header_block_fragment_size(&header, &hpl);
            // We are reading a new header frame, so previous fragments are useless
            if(st->h2s.header_block_fragments_pointer != 0){
              st->h2s.header_block_fragments_pointer = 0;
            }
            // We check if hbf fits on buffer
            if(hbf_size >= HTTP2_MAX_HBF_BUFFER){
              ERROR("Header block fragments too big (not enough space allocated). INTERNAL_ERROR");
              return -1;
            }
            //first we receive fragments, so we save those on the st->h2s.header_block_fragments buffer
            rc = buffer_copy(st->h2s.header_block_fragments, hpl.header_block_fragment, hbf_size, st->h2s.header_block_fragments_pointer);
            if(rc < 1){
              ERROR("Headers' header block fragment were not written or paylaod was empty");
              return -1;
            }
            st->h2s.header_block_fragments_pointer += rc;
            //If end_stream is received-> wait for an end headers (in header or continuation) to half_close the stream
            if(is_flag_set(header.flags,HEADERS_END_STREAM_FLAG)){
                st->h2s.received_end_stream = 1;
            }
            //when receive (continuation or header) frame with flag end_header then the fragments can be decoded, and the headers can be obtained.
            if(is_flag_set(header.flags,HEADERS_END_HEADERS_FLAG)){
                //return number of headers written on header_list, so http2 can update table_count
                rc = receive_header_block(st->h2s.header_block_fragments, st->h2s.header_block_fragments_pointer,st->header_list, st->table_count);
                if(rc < 1){
                  ERROR("Error was found receiving header_block");
                  return -1;
                }
                st->table_count = rc;
                st->h2s.waiting_for_end_headers_flag = 0;//RESET TO 0
                if(st->h2s.received_end_stream == 1){
                    st->h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
                    st->h2s.received_end_stream = 0;//RESET TO 0
                }
                uint32_t header_list_size = get_header_list_size(st->header_list, st->table_count);
                uint32_t MAX_HEADER_LIST_SIZE_VALUE = get_setting_value(st->h2s.local_settings,MAX_HEADER_LIST_SIZE);
                if (header_list_size > MAX_HEADER_LIST_SIZE_VALUE) {
                  ERROR("Header list size greater than max alloweed. Send HTTP 431");
                  //TODO send error and finish stream
                  return 0;
                }
                //we notify http that new headers were written
                st->new_headers = 1;
                st->keep_receiving = 0;
            }
            return 0;
        }
        case PRIORITY_TYPE://Priority
            WARN("TODO: Priority Frame. Not implemented yet.");
            return -1;
        case RST_STREAM_TYPE://RST_STREAM
            WARN("TODO: Reset Stream Frame. Not implemented yet.");
            return -1;
        case SETTINGS_TYPE:{//Settings
            rc = check_for_settings_ack(&header, st);
            if(rc < 0){
                ERROR("Error was found in SETTINGS Header");
                return -1;
            }
            else if(rc){ /*Frame was an ACK*/
                return 0;
            }
            settings_payload_t spl;
            settings_pair_t pairs[header.length/6];
            rc = handle_settings_payload(buff_read, &header, &spl, pairs, st);
            if(rc == -1){
                ERROR("Error in read settings payload");
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
            //returns -1 if protocol error, -2 if stream closed, 0 if no errors
            rc = check_incoming_continuation_condition(&header, st);
            if(rc == -1){
              ERROR("PROTOCOL ERROR was found");
              return -1;
            }
            else if(rc == -2){
              ERROR("STREAM CLOSED ERROR was found");
              return -1;
            }
            continuation_payload_t contpl;
            uint8_t continuation_block_fragment[64];
            // We check if header length fits on array, so there is no segmentation fault on read_continuation_payload
            if(header.length >=64){
              ERROR("Error block fragments too big (not enough space allocated). INTERNAL ERROR");
              return -1;
            }
            int rc = read_continuation_payload(buff_read, &header, &contpl, continuation_block_fragment);
            if(rc < 1){
              ERROR("Error in continuation payload reading");
              return -1;
            }
            //We check if payload fits on buffer
            if(header.length >= HTTP2_MAX_HBF_BUFFER - st->h2s.header_block_fragments_pointer){
              ERROR("Continuation Header block fragments doesnt fit on buffer (not enough space allocated). INTERNAL ERROR");
              return -1;
            }
            //receive fragments and save those on the st->h2s.header_block_fragments buffer
            rc = buffer_copy(st->h2s.header_block_fragments, contpl.header_block_fragment, header.length, st->h2s.header_block_fragments_pointer);
            if(rc < 1){
              ERROR("Continuation block fragment was not written or payload was empty");
              return -1;
            }
            st->h2s.header_block_fragments_pointer += rc;
            if(is_flag_set(header.flags, CONTINUATION_END_HEADERS_FLAG)){
                //return number of headers written on header_list, so http2 can update table_count
                rc = receive_header_block(st->h2s.header_block_fragments, st->h2s.header_block_fragments_pointer,st->header_list, st->table_count);
                st->table_count = rc;
                st->h2s.waiting_for_end_headers_flag = 0;
                if(st->h2s.received_end_stream == 1){ //IF RECEIVED END_STREAM IN HEASDER FRAME, THEN CLOSE THE STREAM
                    st->h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
                    st->h2s.received_end_stream = 0;//RESET TO 0
                }
                uint32_t header_list_size = get_header_list_size(st->header_list, st->table_count);
                uint32_t MAX_HEADER_LIST_SIZE_VALUE = get_setting_value(st->h2s.local_settings,MAX_HEADER_LIST_SIZE);
                if (header_list_size > MAX_HEADER_LIST_SIZE_VALUE) {
                  ERROR("Header list size greater than max alloweed. Send HTTP 431");
                  //TODO send error and finish stream
                  return 0;
                }
                // notify http for reading
                st->new_headers = 1;
                st->keep_receiving = 0;
            }
            return 0;
        }
        default:
            WARN("Error: Type not found");
            return -1;
    }
}

/*
* Function: h2_send_headers
* Given an hstates struct, builds and sends a message to endpoint. The message
* is a sequence of a HEADER FRAME followed by 0 or more CONTINUATION FRAMES.
* Input: -> st: hstates_t struct where headers are written
* Output: 0 if process was made successfully, -1 if not.
*/
int h2_send_headers(hstates_t *st){
  (void) st;
  return -1;
}
