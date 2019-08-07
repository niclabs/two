#include "http2.h"
#include "headers.h"
//#define LOG_LEVEL LOG_LEVEL_INFO
#include "logging.h"
#include "utils.h"
#include "http2_flowcontrol.h"


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

    st->h2s.header_block_fragments_pointer = 0;
    st->h2s.waiting_for_end_headers_flag = 0;

    st->h2s.remote_settings[0] = st->h2s.local_settings[0] = DEFAULT_HEADER_TABLE_SIZE;
    st->h2s.remote_settings[1] = st->h2s.local_settings[1] = DEFAULT_ENABLE_PUSH;
    st->h2s.remote_settings[2] = st->h2s.local_settings[2] = DEFAULT_MAX_CONCURRENT_STREAMS;
    st->h2s.remote_settings[3] = st->h2s.local_settings[3] = DEFAULT_INITIAL_WINDOW_SIZE;
    st->h2s.remote_settings[4] = st->h2s.local_settings[4] = DEFAULT_MAX_FRAME_SIZE;
    st->h2s.remote_settings[5] = st->h2s.local_settings[5] = DEFAULT_MAX_HEADER_LIST_SIZE;
    st->h2s.wait_setting_ack = 0;
    st->h2s.current_stream.stream_id = st->is_server ? 2 : 3;
    st->h2s.current_stream.state = STREAM_IDLE;
    st->h2s.last_open_stream_id = 1;

    st->h2s.incoming_window.window_size = DEFAULT_INITIAL_WINDOW_SIZE;
    st->h2s.incoming_window.window_used = 0;

    st->h2s.outgoing_window.window_size = DEFAULT_INITIAL_WINDOW_SIZE;
    st->h2s.outgoing_window.window_used = 0;

    st->h2s.sent_goaway = 0;
    st->h2s.received_goaway = 0;
    st->h2s.debug_size = 0;

    return 0;
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
        ERROR("Error reading bytes from http, read %d bytes", rc);
        return -1;
    }
    /*Must be 0*/
    rc = bytes_to_frame_header(buff_read, 9, header);
    if(rc){
        ERROR("Error coding bytes to frame header");
        return -1;
    }
    if(header->length > 256){
        printf("Error: Payload's size (%u) too big (>256)\n", header->length);
        return -1;
    }
    rc = read_n_bytes(buff_read, header->length, st);
    if(rc != header->length){
        ERROR("Error reading bytes from http");
        return -1;
    }
    return 0;
}

/*
* Function: update_settings_table
* Updates the specified table of settings with a given settings payload.
* Input: -> spl: settings_payload_t pointer where settings values are stored
*        -> place: must be LOCAL or REMOTE. It indicates which table to update.
         -> st: pointer to hstates_t struct where settings table are stored.
* Output: 0 if update was successfull, -1 if not
*/
int update_settings_table(settings_payload_t *spl, uint8_t place, hstates_t *st){
    uint8_t i;
    uint16_t id;
    int rc = 0;
    /*Verify the values of settings*/
    for(i = 0; i < spl->count; i++){
        rc += verify_setting(spl->pairs[i].identifier, spl->pairs[i].value);
    }
    if(rc != 0){
        ERROR("Error: invalid setting found");
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
        ERROR("Error: Not a valid table to update");
        return -1;
    }
}

/*
* Function: send_local_settings
* Sends local settings to endpoint.
* Input: -> st: pointer to hstates_t struct where local settings are stored
* Output: 0 if settings were sent. -1 if not.
*/
int send_local_settings(hstates_t *st){
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
        ERROR("Error in Settings Frame creation");
        return -1;
    }
    uint8_t byte_mysettings[9+6*6]; /*header: 9 bytes + 6 * setting: 6 bytes */
    int size_byte_mysettings = frame_to_bytes(&mysettingframe, byte_mysettings);
    /*Assuming that http_write returns the number of bytes written*/
    rc = http_write(st, byte_mysettings, size_byte_mysettings);
    INFO("Sending settings");
    if(rc != size_byte_mysettings){
        ERROR("Error in local settings writing");
        return -1;
    }
    /*Settings were sent, so we expect an ack*/
    st->h2s.wait_setting_ack = 1;
    return 0;
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
    if(rc < 0){
        ERROR("Error in Settings ACK creation!");
        return -1;
    }
    uint8_t byte_ack[9+0]; /*Settings ACK frame only has a header*/
    int size_byte_ack = frame_to_bytes(&ack_frame, byte_ack);
    rc = http_write(st, byte_ack, size_byte_ack);
    INFO("Sending settings ACK");
    if(rc != size_byte_ack){
        ERROR("Error in Settings ACK sending");
        return -1;
    }
    return 0;
}

/*
* Function: check_incoming_settings_condition
* Verifies the correctness of header and checks if frame settings is an ACK.
* Input: -> header: pointer to settings frame header to read
*        -> st: pointer to hstates struct where connection variables are stored
* Output: 0 if ACK was not setted. 1 if it was. -1 if error was found.
*/
int check_incoming_settings_condition(frame_header_t *header, hstates_t *st){
    if(header->stream_id != 0){
        ERROR("Settings frame stream id is not zero. PROTOCOL ERROR");
        return -1;
    }
    else if(header->length > read_setting_from(st, LOCAL, MAX_FRAME_SIZE)){
      ERROR("Settings payload bigger than allowed. MAX_FRAME_SIZE ERROR");
      return -1;
    }
    /*Check if ACK is set*/
    if(is_flag_set(header->flags, SETTINGS_ACK_FLAG)){
        if(header->length != 0){
            ERROR("Settings payload size is not zero. PROTOCOL ERROR");
            return -1;
        }
        else{
            if(st->h2s.wait_setting_ack){
                st->h2s.wait_setting_ack = 0;
                return 1;
            }
            else{
                WARN("ACK received but not expected");
                return 1;
            }
        }
    }
    else{
        return 0;
    }
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
int handle_settings_payload(settings_payload_t *spl, hstates_t *st){
    if(!update_settings_table(spl, REMOTE, st)){
        send_settings_ack(st);
        return 0;
    }
    else{
        /*TODO: send protocol error*/
        ERROR("PROTOCOL ERROR: there was an error in settings payload");
        return -1;
    }
}

/*
* Function: send_goaway
* Given an hstates_t struct and a valid error code, sends a GOAWAY FRAME to endpoint.
* If additional debug data (opaque data) is included it must be written on the
* given hstates_t.h2s.debug_data_buffer buffer with its corresponding size on the
* hstates_t.h2s.debug_size variable.
* IMPORTANT: RFC 7540 section 6.8 indicates that a gracefully shutdown should have
* an 2^31-1 value on the last_stream_id field and wait at least one RTT before sending
* a goaway frame with the last stream id. This implementation doesn't.
* Input: ->st: pointer to hstates_t struct where connection variables are stored
*        ->error_code: error code for GOAWAY FRAME (RFC 7540 section 7)
* Output: 0 if no errors were found, -1 if not
*/
int send_goaway(hstates_t *st, uint32_t error_code){//, uint8_t *debug_data_buff, uint8_t debug_size){
  int rc;
  frame_t frame;
  frame_header_t header;
  goaway_payload_t goaway_pl;
  uint8_t additional_debug_data[st->h2s.debug_size];
  rc = create_goaway_frame(&header, &goaway_pl, additional_debug_data, st->h2s.last_open_stream_id, error_code, st->h2s.debug_data_buffer, st->h2s.debug_size);
  if(rc < 0){
    ERROR("Error creating GOAWAY frame");
    return -1;
  }
  frame.frame_header = &header;
  frame.payload = (void*)&goaway_pl;

  uint8_t buff_bytes[HTTP2_MAX_BUFFER_SIZE];
  int bytes_size = frame_to_bytes(&frame, buff_bytes);
  rc = http_write(st,buff_bytes,bytes_size);
  INFO("Sending GOAWAY");

  if(rc != bytes_size){
    ERROR("Error writting goaway frame. INTERNAL ERROR");
    return -1;
  }
  st->h2s.sent_goaway = 1;
  return 0;

}

/*
* Function: handle_goaway_payload
* Handles go away payload.
* Input: ->goaway_pl: goaway_payload_t pointer to goaway frame payload
*        ->st: pointer hstates_t struct where connection variables are stored
* IMPORTANT: this implementation doesn't check the correctness of the last stream
* id sent by the endpoint. That is, if a previous GOAWAY was received with an n
* last_stream_id, it assumes that the next value received is going to be the same.
* Output: 0 if no error were found during the handling, 1 if not
*/
int handle_goaway_payload(goaway_payload_t *goaway_pl, hstates_t *st){
  if(goaway_pl->error_code != HTTP2_NO_ERROR){
      INFO("Received GOAWAY with ERROR");
      // i guess that is closed on the other side, are you?
      return -1;
  }
  if(st->h2s.sent_goaway == 1){ // received answer to goaway
    st->connection_state = 0;
    INFO("Connection CLOSED");
    return 0;
  }
  if(st->h2s.received_goaway == 1){
    INFO("Another GOAWAY has been received before");
  }
  else { // never has been seen a goaway before in this connection life
    st->h2s.received_goaway = 1; // receiver must not open additional streams
  }
  if(st->h2s.current_stream.stream_id > goaway_pl->last_stream_id){
    if(st->h2s.current_stream.state != STREAM_IDLE){
      st->h2s.current_stream.state = STREAM_CLOSED;
      INFO("Current stream closed");
    }
    int rc = send_goaway(st, HTTP2_NO_ERROR); // We send a goaway to close the connection
    if(rc < 0){
      ERROR("Error sending GOAWAY FRAMES");
      return rc;
    }
  }
  return 0;
}

/*
* Function: change_stream_state_end_stream_flag
* Given an hstates_t struct and a boolean, change the state of the current stream
* when a END_STREAM_FLAG is sent or received.
* Input: ->st: pointer to hstates_t struct where connection variables are stored
*        ->sending: boolean like uint8_t that indicates if current flag is sent or received
* Output: 0 if no errors were found, -1 if not
*/
int change_stream_state_end_stream_flag(hstates_t *st, uint8_t sending){
  int rc = 0;
  if(sending){ // Change stream status if end stream flag is sending
    if(st->h2s.current_stream.state == STREAM_OPEN){
      st->h2s.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
    }
    else if(st->h2s.current_stream.state == STREAM_HALF_CLOSED_REMOTE){
      st->h2s.current_stream.state = STREAM_CLOSED;
      if(st->h2s.received_goaway){
        rc = send_goaway(st, HTTP2_NO_ERROR);
        if(rc < 0){
          ERROR("Error in GOAWAY sending. INTERNAL ERROR");
          return rc;
        }
      }
      else{
        rc = prepare_new_stream(st);
      }
    }
    return rc;
  }
  else{ // Change stream status if send stream flag is received
    if(st->h2s.current_stream.state == STREAM_OPEN){
      st->h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    }
    else if(st->h2s.current_stream.state == STREAM_HALF_CLOSED_LOCAL){
      st->h2s.current_stream.state = STREAM_CLOSED;
      if(st->h2s.received_goaway){
        rc = send_goaway(st, HTTP2_NO_ERROR);
        if(rc < 0){
          ERROR("Error in GOAWAY sending. INTERNAL ERROR");
          return rc;
        }
      }
      else{
        rc = prepare_new_stream(st);
      }
    }
    return rc;
  }
}

/*
* Function: check_incoming_headers_condition
* Checks the incoming frame stream_id and the current stream stream_id and
* verifies its correctness. Creates a new stream if needed.
* Input: -> header: header of the incoming headers frame
*        -> st: hstates_t struct where stream variables are stored
* Ouput: 0 if no errors were found, -1 if not
*/
int check_incoming_headers_condition(frame_header_t *header, hstates_t *st){
  // Check if stream is not created or previous one is closed
  if(st->h2s.waiting_for_end_headers_flag){
    //protocol error
    ERROR("CONTINUATION frame was expected. PROTOCOL ERROR");
    return -1;
  }
  if(header->stream_id == 0){
    ERROR("Invalid stream id: 0. PROTOCOL ERROR");
    return -1;
  }
  if(header->length > read_setting_from(st, LOCAL, MAX_FRAME_SIZE)){
    ERROR("Frame exceeds the MAX_FRAME_SIZE. FRAME SIZE ERROR");
    return -1;
  }
  if(st->h2s.current_stream.state == STREAM_IDLE){
      if(header->stream_id < st->h2s.last_open_stream_id){
        ERROR("Invalid stream id: not bigger than last open. PROTOCOL ERROR");
        return -1;
      }
      if(header->stream_id%2 != st->is_server){
        INFO("Incoming stream id: %u", header->stream_id);
        ERROR("Invalid stream id parity. PROTOCOL ERROR");
        return -1;
      }
      else{ // Open a new stream, update last_open and last_peer stream
        st->h2s.current_stream.stream_id = header->stream_id;
        st->h2s.current_stream.state = STREAM_OPEN;
        st->h2s.last_open_stream_id = st->h2s.current_stream.stream_id;
        return 0;
      }
  }
  else if(st->h2s.current_stream.state != STREAM_OPEN &&
          st->h2s.current_stream.state != STREAM_HALF_CLOSED_LOCAL){
        //stream closed error
        ERROR("Current stream is not open. STREAM CLOSED ERROR");
        return -1;
  }
  else if(header->stream_id != st->h2s.current_stream.stream_id){
      //protocol error
      ERROR("Stream ids do not match. PROTOCOL ERROR");
      return -1;
  }
  else{
    return 0;
  }
}

/*
* Function: handle_headers_payload
* Does all the operations related to an incoming HEADERS FRAME.
* Input: -> header: pointer to the headers frame header (frame_header_t)
*        -> hpl: pointer to the headers payload (headers_payload_t)
*        -> st: pointer to hstates_t struct where connection variables are stored
* Output: 0 if no error was found, -1 if not.
*/
int handle_headers_payload(frame_header_t *header, headers_payload_t *hpl, hstates_t *st){
  // we receive a headers, so it could be continuation frames
  int rc;
  st->h2s.waiting_for_end_headers_flag = 1;
  st->keep_receiving = 1;
  int hbf_size = get_header_block_fragment_size(header, hpl);
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
  rc = buffer_copy(st->h2s.header_block_fragments + st->h2s.header_block_fragments_pointer, hpl->header_block_fragment, hbf_size);
  if(rc < 1){
    ERROR("Headers' header block fragment were not written or paylaod was empty. rc = %d",rc);
    return -1;
  }
  st->h2s.header_block_fragments_pointer += rc;
  //If end_stream is received-> wait for an end headers (in header or continuation) to half_close the stream
  if(is_flag_set(header->flags,HEADERS_END_STREAM_FLAG)){
      st->h2s.received_end_stream = 1;
  }
  //when receive (continuation or header) frame with flag end_header then the fragments can be decoded, and the headers can be obtained.
  if(is_flag_set(header->flags,HEADERS_END_HEADERS_FLAG)){
      //return bytes read.
      rc = receive_header_block(st->h2s.header_block_fragments, st->h2s.header_block_fragments_pointer,&st->headers_in);
      if(rc < 0){
        ERROR("Error was found receiving header_block");
        return -1;
      }
      if(rc!= st->h2s.header_block_fragments_pointer){
          ERROR("ERROR still exists fragments to receive.");
          return -1;
      }
      else{//all fragments already received.
          st->h2s.header_block_fragments_pointer = 0;
      }
      //st->hd_lists.header_list_count_in = rc;
      st->h2s.waiting_for_end_headers_flag = 0;//RESET TO 0
      if(st->h2s.received_end_stream == 1){
          change_stream_state_end_stream_flag(st, 0); //0 is for receiving
          st->h2s.received_end_stream = 0;//RESET TO 0
      }
      uint32_t header_list_size = headers_get_header_list_size(&st->headers_in);
      uint32_t MAX_HEADER_LIST_SIZE_VALUE = read_setting_from(st, LOCAL, MAX_HEADER_LIST_SIZE);
      if (header_list_size > MAX_HEADER_LIST_SIZE_VALUE) {
        ERROR("Header list size greater than max allowed. Send HTTP 431");
        st->keep_receiving = 0;
        //TODO send error and finish stream
        return 0;
      }
      //we notify http that new headers were written
      st->new_headers = 1;
      st->keep_receiving = 0;
  }
  return 0;
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

/*
* Function: handle_continuation_payload
* Does all the operations related to an incoming CONTINUATION FRAME.
* Input: -> header: pointer to the continuation frame header (frame_header_t)
*        -> hpl: pointer to the continuation payload (continuation_payload_t)
*        -> st: pointer to hstates_t struct where connection variables are stored
* Output: 0 if no error was found, -1 if not.
*/
int handle_continuation_payload(frame_header_t *header, continuation_payload_t *contpl, hstates_t *st){
  int rc;
  //We check if payload fits on buffer
  if(header->length >= HTTP2_MAX_HBF_BUFFER - st->h2s.header_block_fragments_pointer){
    ERROR("Continuation Header block fragments doesnt fit on buffer (not enough space allocated). INTERNAL ERROR");
    return -1;
  }
  //receive fragments and save those on the st->h2s.header_block_fragments buffer
  rc = buffer_copy(st->h2s.header_block_fragments + st->h2s.header_block_fragments_pointer, contpl->header_block_fragment, header->length);
  if(rc < 1){
    ERROR("Continuation block fragment was not written or payload was empty");
    return -1;
  }
  st->h2s.header_block_fragments_pointer += rc;
  if(is_flag_set(header->flags, CONTINUATION_END_HEADERS_FLAG)){
      //return number of headers written on header_list, so http2 can update header_list_count
      rc = receive_header_block(st->h2s.header_block_fragments, st->h2s.header_block_fragments_pointer,&st->headers_in); //TODO check this: rc is the byte read from the header

      if(rc < 0){
        ERROR("Error was found receiving header_block");
        return -1;
      }
      if(rc!= st->h2s.header_block_fragments_pointer){
          ERROR("ERROR still exists fragments to receive.");
          return -1;
      }
      else{//all fragments already received.
          st->h2s.header_block_fragments_pointer = 0;
      }
      //st->hd_lists.header_list_count_in = rc;
      st->h2s.waiting_for_end_headers_flag = 0;
      if(st->h2s.received_end_stream == 1){ //IF RECEIVED END_STREAM IN HEADER FRAME, THEN CLOSE THE STREAM
          st->h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
          st->h2s.received_end_stream = 0;//RESET TO 0
      }
      uint32_t header_list_size = headers_get_header_list_size(&st->headers_in);
      uint32_t MAX_HEADER_LIST_SIZE_VALUE = read_setting_from(st, LOCAL, MAX_HEADER_LIST_SIZE);
      if (header_list_size > MAX_HEADER_LIST_SIZE_VALUE) {
        WARN("Header list size greater than max allowed. Send HTTP 431");
        st->keep_receiving = 0;
        //TODO send error and finish stream
        return 0;
      }
      // notify http for reading
      st->new_headers = 1;
      st->keep_receiving = 0;
  }
  return 0;
}

/*
* Function: send_data
* Sends a data frame with the current data written in the given hstates_t struct.
* The data is expected to be written in the http_data_t.data_out.buf, and its
* corresponding size indicated in the http_data_t.data_out.size variable.
* Input: ->st: hstates_t struct where connection variables are stored
         ->end_stream: indicates if the data frame to be sent must have the
                      END_STREAM_FLAG set.
* Output: 0 if no error were found in the process, -1 if not
*
*/
int send_data(hstates_t *st, uint8_t end_stream){
    if(st->data_out.size<=0){
        ERROR("no data to be send");
        return -1;
    }
    h2_stream_state_t state = st->h2s.current_stream.state;
    if(state!=STREAM_OPEN && state!=STREAM_HALF_CLOSED_REMOTE){
        ERROR("Wrong state for sending DATA");
        return -1;
    }
    uint32_t stream_id=st->h2s.current_stream.stream_id;//TODO
    uint8_t count_data_to_send = get_size_data_to_send(st);
    frame_t frame;
    frame_header_t frame_header;
    data_payload_t data_payload;
    uint8_t data[count_data_to_send];
    int rc = create_data_frame(&frame_header, &data_payload, data, st->data_out.buf + st->data_out.processed, count_data_to_send, stream_id);
    if(rc<0){
        ERROR("error creating data frame");
        return -1;
    }
    if(end_stream) {
        frame_header.flags = set_flag(frame_header.flags, DATA_END_STREAM_FLAG);
    }

    frame.frame_header = &frame_header;
    frame.payload = (void*)&data_payload;
    uint8_t buff_bytes[HTTP2_MAX_BUFFER_SIZE];
    int bytes_size = frame_to_bytes(&frame, buff_bytes);
    rc = http_write(st,buff_bytes,bytes_size);
    INFO("Sending DATA");

    if(rc != bytes_size){
        ERROR("Error writting data frame. INTERNAL ERROR");
        return rc;
    }
    if(end_stream){
      change_stream_state_end_stream_flag(st, 1); // 1 is for sending
    }
    st->data_out.processed += count_data_to_send;
    if(st->data_out.size == st->data_out.processed) {
        st->data_out.size = 0;
        st->data_out.processed = 0;
    }

    //TODO update window size
    return 0;
}

int check_incoming_data_condition(frame_header_t *header, hstates_t *st){
    if(st->h2s.current_stream.stream_id == 0){
      ERROR("Data stream ID is 0. PROTOCOL ERROR");
      return -1;
    }
    else if(header->length > read_setting_from(st, LOCAL, MAX_FRAME_SIZE)){
      ERROR("Data payload bigger than allower. MAX_FRAME_SIZE error");
      return -1;
    }
    else if(header->stream_id > st->h2s.current_stream.stream_id){
      ERROR("Stream ID is invalid. PROTOCOL ERROR");
      return -1;
    }
    else if(header->stream_id < st->h2s.current_stream.stream_id){
      ERROR("Stream closed. STREAM CLOSED ERROR");
      return -1;
    }
    else if(st->h2s.current_stream.state == STREAM_IDLE){
      ERROR("Stream was in IDLE state. PROTOCOL ERROR");
      return -1;
    }
    else if(st->h2s.current_stream.state != STREAM_OPEN && st->h2s.current_stream.state != STREAM_HALF_CLOSED_REMOTE){
      ERROR("Stream was not in a valid state for data. STREAM CLOSED ERROR");
      return -1;
    }
    return 0;
}

int handle_data_payload(frame_header_t* frame_header, data_payload_t* data_payload, hstates_t* st) {
    uint32_t data_length = frame_header->length;//padding not implemented(-data_payload->pad_length-1 if pad_flag_set)
    /*check flow control*/
    //TODO flow control
    int rc = flow_control_receive_data(st, data_length);
    if(rc < 0){
        ERROR("flow control error");
        return -1;
    }
    buffer_copy(st->data_in.buf + st->data_in.size, data_payload->data, data_length);
    st->data_in.size += data_length;
    if (is_flag_set(frame_header->flags, DATA_END_STREAM_FLAG)){
        st->h2s.received_end_stream = 1;
    }
    // Stream state handling for end stream flag
    if(st->h2s.received_end_stream == 1){
        change_stream_state_end_stream_flag(st, 0); // 0 is for receiving
        st->h2s.received_end_stream = 0;
    }
    return 0;
}

/*
* Function: send_headers_stream_verification
* Given an hstates struct, checks the current stream state and uses it, creates
* a new one or reports an error. The stream that will be used is stored in
* st->h2s.current_stream.stream_id .
* Input: ->st: hstates_t struct where current stream is stored
* Output: 0 if no errors were found, -1 if not
*/
int send_headers_stream_verification(hstates_t *st){
  if(st->h2s.current_stream.state == STREAM_CLOSED ||
      st->h2s.current_stream.state == STREAM_HALF_CLOSED_LOCAL){
      ERROR("Current stream was closed! Send request error. STREAM CLOSED ERROR");
      return -1;
  }
  else if(st->h2s.current_stream.state == STREAM_IDLE){
    if(st->is_server){ // server must use even numbers
        st->h2s.last_open_stream_id += st->h2s.last_open_stream_id%2 ? 1 : 2;
    }
    else{ //stream is closed and id is not zero
        st->h2s.last_open_stream_id += st->h2s.last_open_stream_id%2 ? 2 : 1;
    }
    st->h2s.current_stream.state = STREAM_OPEN;
    st->h2s.current_stream.stream_id = st->h2s.last_open_stream_id;
  }
  return 0;
}

int send_window_update(hstates_t *st, uint8_t window_size_increment){
    h2_stream_state_t state = st->h2s.current_stream.state;
    if(state!=STREAM_OPEN && state!=STREAM_HALF_CLOSED_REMOTE){
        ERROR("Wrong state. ");
        return -1;
    }
    uint32_t stream_id=st->h2s.current_stream.stream_id;//TODO

    frame_t frame;
    frame_header_t frame_header;
    window_update_payload_t window_update_payload;
    int rc = create_window_update_frame(&frame_header, &window_update_payload,window_size_increment,stream_id);
    if(rc<0){
        ERROR("error creating window_update frame");
        return -1;
    }
    if(window_size_increment > st->h2s.incoming_window.window_used){
        ERROR("Trying to send window increment greater than used");
        return -1;
    }
    frame.frame_header = &frame_header;
    frame.payload = (void*)&window_update_payload;
    uint8_t buff_bytes[HTTP2_MAX_BUFFER_SIZE];
    int bytes_size = frame_to_bytes(&frame, buff_bytes);
    rc = http_write(st,buff_bytes,bytes_size);
    INFO("Sending WINDOW UPDATE");

    if(rc != bytes_size){
        ERROR("Error writting window_update frame. INTERNAL ERROR");
        return rc;
    }
    rc = flow_control_send_window_update(st, window_size_increment);
    if(rc!=0){
        ERROR("ERROR in flow control when sending WU");
        return -1;
    }
    return 0;


}

/*
* Function: send_headers_frame
* Send a single headers frame to endpoint. It read the data from the buff_read
* buffer given as parameter.
* Input: ->st: hstates_t struct where connection variables are stored
*        ->buff_read: buffer where headers frame payload is stored
*        ->size: number of bytes to read from buff_read and to store in payload
*        ->stream_id: stream id to write on headers frame header
*        ->end_headers: boolean that indicates if END_HEADERS_FLAG must be set
*        ->end_stream: boolean that indicates if END_STREAM_FLAG must be set
* Output: 0 if no errors were found during frame creation/sending, -1 if not
*/

int send_headers_frame(hstates_t *st, uint8_t *buff_read, int size, uint32_t stream_id, uint8_t end_headers, uint8_t end_stream){
  int rc;
  frame_t frame;
  frame_header_t frame_header;
  headers_payload_t headers_payload;
  uint8_t header_block_fragment[HTTP2_MAX_BUFFER_SIZE];
  // We create the headers frame
  rc = create_headers_frame(buff_read, size, stream_id, &frame_header, &headers_payload, header_block_fragment);
  if(rc < 0){
    ERROR("Error creating headers frame. INTERNAL ERROR");
    return rc;
  }
  if(end_headers){
    frame_header.flags = set_flag(frame_header.flags, HEADERS_END_HEADERS_FLAG);
  }
  if(end_stream){
    frame_header.flags = set_flag(frame_header.flags, HEADERS_END_STREAM_FLAG);
  }
  frame.frame_header = &frame_header;
  frame.payload = (void*)&headers_payload;
  int bytes_size = frame_to_bytes(&frame, buff_read);
  rc = http_write(st,buff_read,bytes_size);
  INFO("Sending headers");

  if(rc != bytes_size){
    ERROR("Error writting headers frame. INTERNAL ERROR");
    return rc;
  }
  return 0;
}

/*
* Function: send_continuation_frame
* Sends a single continuation frame to endpoint. It reads the data from the
* buff_read buffer given as parameter.
* Input: ->st: hstates_t struct where connection variables are stored.
         ->buff_read: buffer where continuation frame payload is stored
         ->size: number of bytes to read from buff_read and to store in payload
         ->stream_id: stream id to write in continuation payload's header
         ->end_stream: boolean that indicates if END_HEADERS_FLAG must be set
* Output: 0 if no errors were found during the creation or sending, -1 if not
*/

int send_continuation_frame(hstates_t *st, uint8_t *buff_read, int size, uint32_t stream_id, uint8_t end_stream){
  int rc;
  frame_t frame;
  frame_header_t frame_header;
  continuation_payload_t continuation_payload;
  uint8_t header_block_fragment[HTTP2_MAX_BUFFER_SIZE];
  rc = create_continuation_frame(buff_read, size, stream_id, &frame_header, &continuation_payload, header_block_fragment);
  if(rc < 0){
    ERROR("Error creating continuation frame. INTERNAL ERROR");
    return rc;
  }
  if(end_stream){
    frame_header.flags = set_flag(frame_header.flags, HEADERS_END_HEADERS_FLAG);
  }
  frame.frame_header = &frame_header;
  frame.payload = (void*)&continuation_payload;
  int bytes_size = frame_to_bytes(&frame, buff_read);
  rc = http_write(st, buff_read, bytes_size);
  INFO("Sending continuation");
  if(rc != bytes_size){
    ERROR("Error writting continuation frame. INTERNAL ERROR");
    return rc;
  }
  return 0;
}

/*
* Function: send_headers
* Given an hstates struct, builds and sends a message to endpoint. The message
* is a sequence of a HEADER FRAME followed by 0 or more CONTINUATION FRAMES.
* Input: -> st: hstates_t struct where headers are written
        -> end_stream: boolean that indicates if end_stream flag needs to be set
* Output: 0 if process was made successfully, -1 if not.
*/
int send_headers(hstates_t *st, uint8_t end_stream){
  if(st->h2s.received_goaway){
    ERROR("GOAWAY was received. Current process must not open a new stream");
    return -1;
  }
  if(st->headers_out.count == 0){
    ERROR("There are no headers to send");
    return -1;
  }
  uint8_t encoded_bytes[HTTP2_MAX_BUFFER_SIZE];
  int size = compress_headers(&st->headers_out, encoded_bytes);
  if(size < 0){
    ERROR("Error was found compressing headers. INTERNAL ERROR");
    return -1;
  }
  if(send_headers_stream_verification(st) < 0){
    ERROR("Stream error during the headers sending. INTERNAL ERROR");
    return -1;
  }
  uint32_t stream_id = st->h2s.current_stream.stream_id;
  uint16_t max_frame_size = read_setting_from(st, LOCAL, MAX_FRAME_SIZE);
  int rc;
  //not being considered dependencies nor padding.
  if(size <= max_frame_size){ //if headers can be send in only one frame
      //only send 1 header
      rc = send_headers_frame(st, encoded_bytes, size, stream_id, 1, end_stream);
      if(rc < 0){
        ERROR("Error found sending headers frame");
        return rc;
      }
      if(end_stream){
        change_stream_state_end_stream_flag(st, 1); // 1 is for sending
      }
      return rc;
  }
  else{//if headers must be send with one or more continuation frames
      int remaining = size;
      //send Header Frame
      rc = send_headers_frame(st, encoded_bytes, max_frame_size, stream_id, 0, end_stream);
      if(rc < 0){
        ERROR("Error found sending headers frame");
        return rc;
      }
      remaining -= max_frame_size;
      //Send continuation Frames
      while(remaining > max_frame_size){
          rc = send_continuation_frame(st,encoded_bytes + (size - remaining), max_frame_size, stream_id, 0);
          if(rc < 0){
            ERROR("Error found sending continuation frame");
            return rc;
          }
          remaining -= max_frame_size;
      }
      //send last continuation frame
      rc = send_continuation_frame(st,encoded_bytes + (size - remaining), remaining, stream_id, 1);
      if(rc < 0){
        ERROR("Error found sending continuation frame");
        return rc;
      }
      if(end_stream){
        change_stream_state_end_stream_flag(st, 1); // 1 is for sending
      }
      return rc;
  }
}

/*----- API Methods ------*/

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
    INFO("Sending preface");
    if(rc != 24){
        ERROR("Error in preface sending");
        return -1;
    }
    puts("Client: sending local settings...");
    if((rc = send_local_settings(st)) < 0){
        ERROR("Error in local settings sending");
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
        ERROR("Error in reading preface");
        return -1;
    }
    puts("Server: 24 bytes read");
    if(strcmp(preface, (char*)preface_buff) != 0){
        ERROR("Error in preface receiving");
        return -1;
    }
    /*Server sends local settings to endpoint*/
    puts("Server: sending local settings...");
    if((rc = send_local_settings(st)) < 0){
        ERROR("Error in local settings sending");
        return -1;
    }
    puts("Server: init connection done");
    return 0;
}

/*
* Function: h2_receive_frame
* Receives a frame from endpoint, decodes it and works with it.
* Input: -> st: pointer to hstates_t struct where connection variables are stored
* Output: 0 if no problem was found. -1 if error was found.
*/
int h2_receive_frame(hstates_t *st){
    INFO("h2_receive_frame");

    uint8_t buff_read[HTTP2_MAX_BUFFER_SIZE];
    //uint8_t buff_write[HTTP2_MAX_BUFFER_SIZE]
    int rc;
    frame_header_t header;
    rc = read_frame(buff_read, &header, st);
    if(rc == -1){
        ERROR("Error reading frame");
        return -1;
    }
    INFO("receiving %u", (uint8_t)header.type);
    switch(header.type){
        case DATA_TYPE: {//Data
            /*check stream state*/
            INFO("h2_receive_frame: DATA");
            rc = check_incoming_data_condition(&header, st);
            if(rc < 0){
              ERROR("Error was found during data receiving");
              return rc;
            }
            data_payload_t data_payload;
            uint8_t data[header.length];
            int rc = read_data_payload(buff_read, &header, &data_payload, data);
            if(rc < 0){
                ERROR("ERROR reading data payload");
                return -1;
            }
            rc = handle_data_payload(&header, &data_payload, st);
            if(rc < 0){
                ERROR("ERROR in handle receive data");
                return -1;
            }
            return 0;
        }
        case HEADERS_TYPE:{//Header
            INFO("h2_receive_frame: HEADERS");
            rc = check_incoming_headers_condition(&header, st);
            if(rc == -1){
              ERROR("Error was found during headers receiving");
              return -1;
            }
            headers_payload_t hpl;
            uint8_t headers_block_fragment[HTTP2_MAX_HBF_BUFFER];
            uint8_t padding[32];
            rc = read_headers_payload(buff_read, &header, &hpl, headers_block_fragment, padding);
            if(rc == 0 || rc > header.length){
                ERROR("Error in headers payload");
                return rc;
            }
            rc = handle_headers_payload(&header, &hpl, st);
            if(rc == -1){
              ERROR("Error during headers payload handling");
              return rc;
            }
            INFO("received headers ok");
            return 0;
        }
        case PRIORITY_TYPE://Priority
            WARN("TODO: Priority Frame. Not implemented yet.");
            return -1;
        case RST_STREAM_TYPE://RST_STREAM
            WARN("TODO: Reset Stream Frame. Not implemented yet.");
            return -1;
        case SETTINGS_TYPE:{//Settings
            rc = check_incoming_settings_condition(&header, st);
            if(rc < 0){
                ERROR("Error was found in SETTINGS Header");
                return -1;
            }
            else if(rc){ /*Frame was an ACK*/
                INFO("Received settings ACK");
                return 0;
            }
            settings_payload_t spl;
            settings_pair_t pairs[header.length/6];
            rc = bytes_to_settings_payload(buff_read, header.length, &spl, pairs);
            if(rc < 0){
              ERROR("Error reading settings payload");
              return rc;
            }
            rc = handle_settings_payload(&spl, st);
            if(rc == -1){
                ERROR("Error in settings payload handling");
                return -1;
            }
            INFO("received settings");
            return rc;
        }
        case PUSH_PROMISE_TYPE://Push promise
            WARN("TODO: Push promise frame. Not implemented yet.");
            return -1;
        case PING_TYPE://Ping
            WARN("TODO: Ping frame. Not implemented yet.");
            return -1;
        case GOAWAY_TYPE:{//Go Avaw
            INFO("h2_receive_frame: GOAWAY");
            if(header.stream_id != 0x0){
              ERROR("GOAWAY doesnt have STREAM ID 0. PROTOCOL ERROR");
              return -1;
            }
            uint16_t max_frame_size = read_setting_from(st, LOCAL, MAX_FRAME_SIZE);
            if(header.length > max_frame_size){
              ERROR("GOAWAY payload bigger than allowed. MAX_FRAME_SIZE ERROR");
              return -1;
            }
            uint8_t debug_data[max_frame_size - 8];
            goaway_payload_t goaway_pl;
            rc = read_goaway_payload(buff_read, &header, &goaway_pl, debug_data);
            if(rc < 0){
              ERROR("Error in reading goaway payload");
              return -1;
            }
            rc = handle_goaway_payload(&goaway_pl, st);
            if(rc < 0){
              ERROR("Error during goaway handling");
              return -1;
            }
            INFO("Received goaway frame");
            return rc;
        }
        case WINDOW_UPDATE_TYPE: {//Window update
            window_update_payload_t window_update_payload;
            int rc = read_window_update_payload(buff_read, &header, &window_update_payload);
            if (rc < 0) {
                ERROR("Error in reading window_update_payload ");
                return -1;
            }
            uint32_t window_size_increment = window_update_payload.window_size_increment;
            rc = flow_control_receive_window_update(st, window_size_increment);
                if(rc < 0){
                    ERROR("ERROR in flow control, receiving window update");
                    return -1;
                }
            return 0;
        }
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
            rc = handle_continuation_payload(&header, &contpl, st);
            if(rc == -1){
              ERROR("Error was found during continuatin payload handling");
              return rc;
            }
            INFO("received continuation ok");
            return 0;
        }
        default:
            WARN("Error: Type not found");
            return -1;
    }
}

/*
* Function: h2_send_request
* Given a set of headers, generates and sends an http2 message to endpoint. The
* message is a request, so it must create a new stream.
* Input: -> st: pointer to hstates_t struct where headers are stored
* Output: 0 if generation and sent was successfull, -1 if not
*/
int h2_send_request(hstates_t *st){
  if(st->headers_out.count == 0){
    ERROR("There were no headers to write");
    return -1;
  }
  int rc = send_headers(st, 1);
  if(rc < 0){
    ERROR("Error was found sending headers on request");
    return rc;
  }
  return 0;
}

/*
* Function: h2_send_response
* Given a set of headers, generates and sends an http2 message to endpoint. The
* message is a response, so it must be sent through an existent stream, closing
* the current stream.
* Input: -> st: pointer to hstates_t struct where headers are stored
* Output: 0 if generation and sent was successfull, -1 if not
*/
int h2_send_response(hstates_t *st){
  if(st->headers_out.count == 0){
    ERROR("There were no headers to write");
    return -1;
  }
  if(st->data_out.size > 0){
    int rc = send_headers(st, 0);
    if(rc < 0){
      ERROR("Error was found sending headers on response");
      return rc;
    }
    rc = send_data(st, 1);
    if(rc < 0){
      ERROR("Error was found sending data on response");
      return rc;
    }
  }
  else{
    int rc = send_headers(st, 1);
    if(rc < 0){
      ERROR("Error was found sending headers on response");
      return rc;
    }
  }
  return 0;
}

/*
* Function: h2_graceful_connection_shutdown
* Sends a GOAWAY FRAME to endpoint with the the last global open stream ID on it
* and a NO ERROR error code. This means that the current process wants to close
* the current connection with endpoint.
* Input: ->st: hstates_t pointer where connection variables are stored
* Output: 0 if no errors were found during goaway sending, -1 if not.
*/

int h2_graceful_connection_shutdown(hstates_t *st){
  int rc = send_goaway(st, HTTP2_NO_ERROR);
  if(rc < 0){
    ERROR("Error sending GOAWAY FRAME to endpoint.");
  }
  return rc;
}
