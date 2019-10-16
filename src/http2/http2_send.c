#include "http2_send.h"
#include "frames.h"
#include "headers.h"
#include "logging.h"
#include "http2_utils_v2.h"
#include "http2_flowcontrol_v2.h"


/*
* Function: change_stream_state_end_stream_flag
* Given an h2states_t struct and a boolean, change the state of the current stream
* when a END_STREAM_FLAG is sent or received.
* Input: ->st: pointer to h2states_t struct where connection variables are stored
*        ->sending: boolean like uint8_t that indicates if current flag is sent or received
* Output: 0 if no errors were found, -1 if not
*/
int change_stream_state_end_stream_flag(uint8_t sending, cbuf_t *buf_out, h2states_t *h2s){
  int rc = 0;
  if(sending){ // Change stream status if end stream flag is sending
    if(h2s->current_stream.state == STREAM_OPEN){
      h2s->current_stream.state = STREAM_HALF_CLOSED_LOCAL;
    }
    else if(h2s->current_stream.state == STREAM_HALF_CLOSED_REMOTE){
      h2s->current_stream.state = STREAM_CLOSED;
      if(h2s->received_goaway){
        rc = send_goaway(HTTP2_NO_ERROR, buf_out, h2s);
        if(rc < 0){
          ERROR("Error in GOAWAY sending. INTERNAL ERROR");
          return rc;
        }
      }
      else{
        rc = prepare_new_stream(h2s);
      }
    }
    return rc;
  }
  else{ // Change stream status if send stream flag is received
    if(h2s->current_stream.state == STREAM_OPEN){
      h2s->current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    }
    else if(h2s->current_stream.state == STREAM_HALF_CLOSED_LOCAL){
      h2s->current_stream.state = STREAM_CLOSED;
      if(h2s->received_goaway){
        rc = send_goaway(HTTP2_NO_ERROR, buf_out, h2s);
        if(rc < 0){
          ERROR("Error in GOAWAY sending. INTERNAL ERROR");
          return rc;
        }
      }
      else{
        rc = prepare_new_stream(h2s);
      }
    }
    return rc;
  }
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
int send_data(uint8_t end_stream, cbuf_t *buf_out, h2states_t *h2s){
    if(h2s->data.size<=0){
        ERROR("No data to be sent. INTERNAL_ERROR");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return -1;
    }
    h2_stream_state_t state = h2s->current_stream.state;
    if(state!=STREAM_OPEN && state!=STREAM_HALF_CLOSED_REMOTE){
        ERROR("Wrong state for sending DATA. INTERNAL_ERROR");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return -1;
    }
    uint32_t stream_id = h2s->current_stream.stream_id;
    uint8_t count_data_to_send = get_size_data_to_send(h2s);
    frame_t frame;
    frame_header_t frame_header;
    data_payload_t data_payload;
    uint8_t data[count_data_to_send];
    int rc = create_data_frame(&frame_header, &data_payload, data, h2s->data.buf + h2s->data.processed, count_data_to_send, stream_id);
    if(rc<0){
        ERROR("Error creating data frame. INTERNAL_ERROR");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return -1;
    }
    if(end_stream) {
        frame_header.flags = set_flag(frame_header.flags, DATA_END_STREAM_FLAG);
    }
    frame.frame_header = &frame_header;
    frame.payload = (void*)&data_payload;
    uint8_t buff_bytes[HTTP2_MAX_BUFFER_SIZE];
    int bytes_size = frame_to_bytes(&frame, buff_bytes);
    INFO("Sending DATA");
    rc = cbuf_push(buf_out, buff_bytes, bytes_size);
    if(rc != bytes_size){
        ERROR("Error writting data frame. INTERNAL ERROR");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return rc;
    }
    rc = flow_control_send_data(h2s, count_data_to_send);
    if(rc < 0){
      send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
      return -1;
    }
    if(end_stream){
      change_stream_state_end_stream_flag(1, buf_out, h2s); // 1 is for sending
    }
    h2s->data.processed += count_data_to_send;
    if(h2s->data.size == h2s->data.processed) {
        h2s->data.size = 0;
        h2s->data.processed = 0;
    }
    return 0;
}


/*
* Function: send_settings_ack
* Sends an ACK settings frame to endpoint
* Input: -> st: pointer to hstates struct where http and http2 connection info is
* stored
* Output: 0 if sent was successfully made, -1 if not.
*/
int send_settings_ack(cbuf_t *buf_out, h2states_t *h2s){
    frame_t ack_frame;
    frame_header_t ack_frame_header;
    int rc;
    rc = create_settings_ack_frame(&ack_frame, &ack_frame_header);
    if(rc < 0){
        ERROR("Error in Settings ACK creation!");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return -1;
    }
    uint8_t byte_ack[9+0]; /*Settings ACK frame only has a header*/
    int size_byte_ack = frame_to_bytes(&ack_frame, byte_ack);
    // We write the ACK to NET
    rc = cbuf_push(buf_out, byte_ack, size_byte_ack);
    INFO("Sending settings ACK");
    if(rc != size_byte_ack){
        ERROR("Error in Settings ACK sending");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return -1;
    }
    return 0;
}

/*
* Function: send_goaway
* Given an h2states_t struct and a valid error code, sends a GOAWAY FRAME to endpoint.
* If additional debug data (opaque data) is included it must be written on the
* given h2states_t.h2s.debug_data_buffer buffer with its corresponding size on the
* h2states_t.h2s.debug_size variable.
* IMPORTANT: RFC 7540 section 6.8 indicates that a gracefully shutdown should have
* an 2^31-1 value on the last_stream_id field and wait at least one RTT before sending
* a goaway frame with the last stream id. This implementation doesn't.
* Input: ->st: pointer to h2states_t struct where connection variables are stored
*        ->error_code: error code for GOAWAY FRAME (RFC 7540 section 7)
* Output: 0 if no errors were found, -1 if not
*/
int send_goaway(uint32_t error_code, cbuf_t *buf_out, h2states_t *h2s){//, uint8_t *debug_data_buff, uint8_t debug_size){
  int rc;
  frame_t frame;
  frame_header_t header;
  goaway_payload_t goaway_pl;
  uint8_t additional_debug_data[h2s->debug_size];
  rc = create_goaway_frame(&header, &goaway_pl, additional_debug_data, h2s->last_open_stream_id, error_code, h2s->debug_data_buffer, h2s->debug_size);
  if(rc < 0){
    ERROR("Error creating GOAWAY frame");
    //TODO shutdown connection
    return -1;
  }
  frame.frame_header = &header;
  frame.payload = (void*)&goaway_pl;
  uint8_t buff_bytes[HTTP2_MAX_BUFFER_SIZE];
  int bytes_size = frame_to_bytes(&frame, buff_bytes);
  rc = cbuf_push(buf_out, buff_bytes, bytes_size);
  INFO("Sending GOAWAY, error code: %u", error_code);

  if(rc != bytes_size){
    ERROR("Error writting goaway frame. INTERNAL ERROR");
    //TODO shutdown connection
    return -1;
  }
  h2s->sent_goaway = 1;
  if(h2s->received_goaway){
    return -1;
  }
  return 0;

}

/*
* Function: send_window_update
* Sends connection window update to endpoint.
* Input: -> st: hstates_t struct pointer where connection variables are stored
*        -> window_size_increment: increment to put on window_update frame
* Output: 0 if no errors were found, -1 if not.
*/
int send_window_update(uint8_t window_size_increment, cbuf_t *buf_out, h2states_t *h2s){
    frame_t frame;
    frame_header_t frame_header;
    window_update_payload_t window_update_payload;
    int rc = create_window_update_frame(&frame_header, &window_update_payload, window_size_increment,0);
    if(rc<0){
        ERROR("error creating window_update frame");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return -1;
    }
    if(window_size_increment > h2s->incoming_window.window_used){
        ERROR("Trying to send window increment greater than used");
        return -1;
    }
    frame.frame_header = &frame_header;
    frame.payload = (void*)&window_update_payload;
    uint8_t buff_bytes[HTTP2_MAX_BUFFER_SIZE];
    int bytes_size = frame_to_bytes(&frame, buff_bytes);
    rc = cbuf_push(buf_out, buff_bytes, bytes_size);

    INFO("Sending WINDOW UPDATE");

    if(rc != bytes_size){
        ERROR("Error writting window_update frame. INTERNAL ERROR");
        return rc;
    }
    rc = flow_control_send_window_update(h2s, window_size_increment);
    if(rc!=0){
        ERROR("ERROR in flow control when sending WU");
        return -1;
    }
    return 0;
}

/*
* Function: send_headers_stream_verification
* Given an hstates struct, checks the current stream and uses it, creates
* a new one or reports an error. The stream that will be used is stored in
* st->h2s.current_stream.stream_id .
* Input: ->st: hstates_t struct where current stream is stored
* Output: 0 if no errors were found, -1 if not
*/
int send_headers_stream_verification(cbuf_t *buf_out, h2states_t *h2s){
  if(h2s->current_stream.state == STREAM_CLOSED ||
      h2s->current_stream.state == STREAM_HALF_CLOSED_LOCAL){
      ERROR("Current stream was closed! Send request error. INTERNAL_ERROR");
      send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
      return -1;
  }
  else if(h2s->current_stream.state == STREAM_IDLE){
    if(h2s->is_server){ // server must use even numbers
        h2s->last_open_stream_id += h2s->last_open_stream_id%2 ? 1 : 2;
    }
    else{ //stream is closed and id is not zero
        h2s->last_open_stream_id += h2s->last_open_stream_id%2 ? 2 : 1;
    }
    h2s->current_stream.state = STREAM_OPEN;
    h2s->current_stream.stream_id = h2s->last_open_stream_id;
  }
  return 0;
}

/*
* Function: send_local_settings
* Sends local settings to endpoint.
* Input: -> st: pointer to hstates_t struct where local settings are stored
* Output: 0 if settings were sent. -1 if not.
*/
int send_local_settings(cbuf_t *buf_out, h2states_t *h2s){
    int rc;
    uint16_t ids[6] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6};
    frame_t mysettingframe;
    frame_header_t mysettingframeheader;
    settings_payload_t mysettings;
    settings_pair_t mypairs[6];
    /*rc must be 0*/
    rc = create_settings_frame(ids, h2s->local_settings, 6, &mysettingframe,
                               &mysettingframeheader, &mysettings, mypairs);
    if(rc){
        ERROR("Error in Settings Frame creation");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return -1;
    }
    uint8_t byte_mysettings[9+6*6]; /*header: 9 bytes + 6 * setting: 6 bytes */
    int size_byte_mysettings = frame_to_bytes(&mysettingframe, byte_mysettings);
    rc = cbuf_push(buf_out, byte_mysettings, size_byte_mysettings);
    INFO("Sending settings");
    if(rc != size_byte_mysettings){
        ERROR("Error in local settings writing");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return -1;
    }
    /*Settings were sent, so we expect an ack*/
    h2s->wait_setting_ack = 1;
    return 0;
}

/*
* Function: send_connection_error
* Send a connection error to endpoint with a specified error code. It implements
* the behaviour suggested in RFC 7540, secion 5.4.1
* Input: -> st: h2states_t pointer where connetion variables are stored
*        -> error_code: error code that will be used to shutdown the connection
* Output: void
*/

void send_connection_error(cbuf_t *buf_out, uint32_t error_code, h2states_t *h2s){
  int rc = send_goaway(error_code, buf_out, h2s);
  if(rc < 0){
    WARN("Error sending GOAWAY frame to endpoint.");
  }
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

int send_continuation_frame(uint8_t *buff_read, int size, uint32_t stream_id, uint8_t end_stream, cbuf_t *buf_out, h2states_t *h2s){
  int rc;
  frame_t frame;
  frame_header_t frame_header;
  continuation_payload_t continuation_payload;
  uint8_t header_block_fragment[HTTP2_MAX_BUFFER_SIZE];
  rc = create_continuation_frame(buff_read, size, stream_id, &frame_header, &continuation_payload, header_block_fragment);
  if(rc < 0){
    ERROR("Error creating continuation frame. INTERNAL ERROR");
    send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
    return rc;
  }
  if(end_stream){
    frame_header.flags = set_flag(frame_header.flags, HEADERS_END_HEADERS_FLAG);
  }
  frame.frame_header = &frame_header;
  frame.payload = (void*)&continuation_payload;
  int bytes_size = frame_to_bytes(&frame, buff_read);
  rc = cbuf_push(buf_out, buff_read, bytes_size);
  INFO("Sending continuation");
  if(rc != bytes_size){
    ERROR("Error writting continuation frame. INTERNAL ERROR");
    send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
    return rc;
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

int send_headers_frame(uint8_t *buff_read, int size, uint32_t stream_id, uint8_t end_headers, uint8_t end_stream, cbuf_t *buf_out, h2states_t *h2s){
  int rc;
  frame_t frame;
  frame_header_t frame_header;
  headers_payload_t headers_payload;
  uint8_t header_block_fragment[HTTP2_MAX_BUFFER_SIZE];
  // We create the headers frame
  rc = create_headers_frame(buff_read, size, stream_id, &frame_header, &headers_payload, header_block_fragment);
  if(rc < 0){
    ERROR("Error creating headers frame. INTERNAL ERROR");
    send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
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
  rc = cbuf_push(buf_out, buff_read, bytes_size);
  INFO("Sending headers");
  if(rc != bytes_size){
    ERROR("Error writting headers frame. INTERNAL ERROR");
    send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
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
int send_headers(uint8_t end_stream, cbuf_t *buf_out, h2states_t *h2s){
  if(h2s->received_goaway){
    ERROR("GOAWAY was received. Current process must not open a new stream");
    return -1;
  }
  if(h2s->headers.count == 0){
    ERROR("There are no headers to send");
    return -1;
  }
  uint8_t encoded_bytes[HTTP2_MAX_BUFFER_SIZE];
  int size = compress_headers(&h2s->headers, encoded_bytes, &h2s->hpack_states);
  if(size < 0){
    ERROR("Error was found compressing headers. INTERNAL ERROR");
    send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
    return -1;
  }
  if(send_headers_stream_verification(buf_out, h2s) < 0){
    ERROR("Stream error during the headers sending. INTERNAL ERROR"); // error already handled
    return -1;
  }
  uint32_t stream_id = h2s->current_stream.stream_id;
  uint16_t max_frame_size = read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE);
  int rc;
  //not being considered dependencies nor padding.
  if(size <= max_frame_size){ //if headers can be send in only one frame
      //only send 1 header
      rc = send_headers_frame(encoded_bytes, size, stream_id, 1, end_stream, buf_out, h2s);
      if(rc < 0){
        ERROR("Error found sending headers frame");
        return rc;
      }
      if(end_stream){
        change_stream_state_end_stream_flag(1, buf_out, h2s); // 1 is for sending
      }
      return rc;
  }
  else{//if headers must be send with one or more continuation frames
      int remaining = size;
      //send Header Frame
      rc = send_headers_frame(encoded_bytes, max_frame_size, stream_id, 0, end_stream, buf_out, h2s);
      if(rc < 0){
        ERROR("Error found sending headers frame");
        return rc;
      }
      remaining -= max_frame_size;
      //Send continuation Frames
      while(remaining > max_frame_size){
          rc = send_continuation_frame(encoded_bytes + (size - remaining), max_frame_size, stream_id, 0, buf_out, h2s);
          if(rc < 0){
            ERROR("Error found sending continuation frame");
            return rc;
          }
          remaining -= max_frame_size;
      }
      //send last continuation frame
      rc = send_continuation_frame(encoded_bytes + (size - remaining), remaining, stream_id, 1, buf_out, h2s);
      if(rc < 0){
        ERROR("Error found sending continuation frame");
        return rc;
      }
      if(end_stream){
        change_stream_state_end_stream_flag(1, buf_out, h2s); // 1 is for sending
      }
      return rc;
  }
}
