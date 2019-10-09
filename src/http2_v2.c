#include "http2_v2.h"
#include "logging.h"
#include <string.h>
#include "http2utils_v2.h"


callback_t h2_server_init_connection(cbuf_t *buf_in, cbuf_t *buf_out, void *state);
callback_t receive_header(cbuf_t *buf_in, cbuf_t *buf_out, void *state);
callback_t receive_payload(cbuf_t *buf_in, cbuf_t *buf_out, void *state);
callback_t receive_payload_wait_settings_ack(cbuf_t *buf_in, cbuf_t *buf_out, void *state);
callback_t receive_payload_goaway(cbuf_t *buf_in, cbuf_t *buf_out, void *state);
int check_incoming_condition(cbuf_t *buf_out, h2states_t *h2s);
int check_incoming_data_condition(cbuf_t *buf_out, h2states_t *h2s);
int check_incoming_headers_condition(cbuf_t *buf_out, h2states_t *h2s);
int check_incoming_settings_condition(cbuf_t *buf_out, h2states_t *h2s);
int check_incoming_goaway_condition(cbuf_t *buf_out, h2states_t *h2s);
int check_incoming_continuation_condition(cbuf_t *buf_out, h2states_t *h2s);
void send_connection_error(cbuf_t *buf_out, uint32_t error_code, h2states_t *h2s);
int handle_payload(cbuf_t *buf_out, h2states_t *h2s);
int handle_data_payload(cbuf_t *buf_out, h2states_t *h2s);
int handle_headers_payload(cbuf_t *buf_out, h2states_t *h2s);
int handle_settings_payload(cbuf_t *buf_out, h2states_t *h2s);
int handle_goaway_payload(cbuf_t *buf_out, h2states_t *h2s);
int handle_window_update_payload(cbuf_t *buf_out, h2states_t *h2s);
int handle_continuation_payload(cbuf_t *buf_out, h2states_t *h2s);
/*
 *
 *
 */
callback_t null_callback(void){
  callback_t null_ret = {NULL, NULL};
  return null_ret;
}

int init_variables_h2s(h2states_t *h2s, uint8_t is_server)
{
    h2s->is_server = is_server;
    h2s->remote_settings[0] = h2s->local_settings[0] = DEFAULT_HEADER_TABLE_SIZE;
    h2s->remote_settings[1] = h2s->local_settings[1] = DEFAULT_ENABLE_PUSH;
    h2s->remote_settings[2] = h2s->local_settings[2] = DEFAULT_MAX_CONCURRENT_STREAMS;
    h2s->remote_settings[3] = h2s->local_settings[3] = DEFAULT_INITIAL_WINDOW_SIZE;
    h2s->remote_settings[4] = h2s->local_settings[4] = DEFAULT_MAX_FRAME_SIZE;
    h2s->remote_settings[5] = h2s->local_settings[5] = DEFAULT_MAX_HEADER_LIST_SIZE;
    h2s->wait_setting_ack = 0;
    h2s->current_stream.stream_id = is_server ? 2 : 3;
    h2s->current_stream.state = STREAM_IDLE;
    h2s->last_open_stream_id = 1;
    h2s->header_block_fragments_pointer = 0;
    h2s->waiting_for_end_headers_flag = 0;
    h2s->received_end_stream = 0;
    h2s->incoming_window.window_size = DEFAULT_INITIAL_WINDOW_SIZE;
    h2s->incoming_window.window_used = 0;
    h2s->outgoing_window.window_size = DEFAULT_INITIAL_WINDOW_SIZE;
    h2s->outgoing_window.window_used = 0;
    h2s->sent_goaway = 0;
    h2s->received_goaway = 0;
    h2s->debug_size = 0;
    //h2s->header = NULL;
    hpack_init_states(&(h2s->hpack_states), DEFAULT_HEADER_TABLE_SIZE);
    return 0;
}

callback_t h2_server_init_connection(cbuf_t *buf_in, cbuf_t *buf_out, void *state)
{
    if (cbuf_len(buf_in) < 24) {
        callback_t ret = { h2_server_init_connection, NULL };
        return ret;
    }
    int rc;
    uint8_t preface_buff[25];
    char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    rc = cbuf_pop(buf_in, preface_buff, 24);
    if (rc != 24) {
        DEBUG("Error during cbuf_pop operation");
        return null_callback();
    }
    preface_buff[24] = '\0';
    h2states_t *h2s = (h2states_t *)state;
    if (strcmp(preface, (char *)preface_buff) != 0) {
        ERROR("Error in preface received");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return null_callback();
    }
    // send connection settings
    rc = init_variables_h2s(h2s, 1);
    callback_t ret_null = { NULL, NULL };
    return ret_null;
}

int validate_pseudoheaders(header_t* pseudoheaders)
{

    if (headers_get(pseudoheaders, ":method") == NULL)
    {
        ERROR("\":method\" pseudoheader was missing.");
        return -1;
    }

    if (headers_get(pseudoheaders, ":scheme") == NULL)
    {
        ERROR("\":scheme\" pseudoheader was missing.");
        return -1;
    }

    if (headers_get(pseudoheaders, ":path") == NULL)
    {
        ERROR("\":path\" pseudoheader was missing.");
        return -1;
    }

    return headers_validate(pseudoheaders);
}

callback_t receive_header(cbuf_t *buf_in, cbuf_t *buf_out, void *state)
{
    if (cbuf_len(buf_in) < 9) {
        callback_t ret = { receive_header, NULL };
        return ret;
    }
    h2states_t *h2s = (h2states_t *)state;
    frame_header_t header;
    uint8_t buff_read_header[10];
    int rc = cbuf_pop(buf_in, buff_read_header, 9);

    if (rc != 9) {
        WARN("READ %d BYTES FROM SOCKET", rc);
        return null_callback();
    }
    rc = bytes_to_frame_header(buff_read_header, 9, &header);
    if (rc) {
        ERROR("Error coding bytes to frame header. INTERNAL_ERROR");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return null_callback();
    }
    if (header.length > read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE)) {
        ERROR("Length of the frame payload greater than expected. FRAME_SIZE_ERROR");
        send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
        return null_callback();
    }
    // save header type
    h2s->header = header;
    // If errors are found, internal logic will handle them.
    rc = check_incoming_condition(buf_out, h2s);
    if(rc < 0){
      DEBUG("Error was found during incoming condition checking");
      return null_callback();
    }
    else if(rc == 1){
      callback_t ret = {receive_header, NULL};
      return ret;
    }
    callback_t ret = { receive_payload, NULL };
    return ret;
}

callback_t receive_payload(cbuf_t *buf_in, cbuf_t *buf_out, void *state)
{   (void) buf_out;
    h2states_t *h2s = (h2states_t *)state;

    if (cbuf_len(buf_in) < h2s->header.length) {
        callback_t ret = { receive_payload, NULL };
        return ret;
    }

    uint8_t buff_read_payload[HTTP2_MAX_BUFFER_SIZE];
    int rc = cbuf_pop(buf_in, buff_read_payload, h2s->header.length);
    if (rc != h2s->header.length) {
        ERROR("Error reading bytes of payload, read %d bytes", rc);
        return null_callback();
    }
    rc = handle_payload(buf_out, h2s);
    // todo: read_type_payload from buff_read_payload

    // placeholder
    callback_t ret_null = { NULL, NULL };
    return ret_null;
}

int check_incoming_data_condition(cbuf_t *buf_out, h2states_t *h2s)
{
    if(h2s->waiting_for_end_headers_flag){
      ERROR("CONTINUATION or HEADERS frame was expected. PROTOCOL ERROR");
      send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
      return -1;
    }
    if(h2s->current_stream.stream_id == 0){
      ERROR("Data stream ID is 0. PROTOCOL ERROR");
      send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
      return -1;
    }
    else if(h2s->header.length > read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE)){
      ERROR("Data payload bigger than allower. MAX_FRAME_SIZE error");
      send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
      return -1;
    }
    else if(h2s->header.stream_id > h2s->current_stream.stream_id){
      ERROR("Stream ID is invalid. PROTOCOL ERROR");
      send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
      return -1;
    }
    else if(h2s->header.stream_id < h2s->current_stream.stream_id){
      ERROR("Stream closed. STREAM CLOSED ERROR");
      send_connection_error(buf_out, HTTP2_STREAM_CLOSED, h2s);
      return -1;
    }
    else if(h2s->current_stream.state == STREAM_IDLE){
      ERROR("Stream was in IDLE state. PROTOCOL ERROR");
      send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
      return -1;
    }
    else if(h2s->current_stream.state != STREAM_OPEN && h2s->current_stream.state != STREAM_HALF_CLOSED_LOCAL){
      ERROR("Stream was not in a valid state for data. STREAM CLOSED ERROR");
      send_connection_error(buf_out, HTTP2_STREAM_CLOSED, h2s);
      return -1;
    }
    return 0;
}


int check_incoming_headers_condition(cbuf_t *buf_out, h2states_t *h2s)
{
  // Check if stream is not created or previous one is closed
  if(h2s->waiting_for_end_headers_flag){
    //protocol error
    ERROR("CONTINUATION frame was expected. PROTOCOL ERROR");
    send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
    return -1;
  }
  if(h2s->header.stream_id == 0){
    ERROR("Invalid stream id: 0. PROTOCOL ERROR");
    send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
    return -1;
  }
  if(h2s->header.length > read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE)){
    ERROR("Frame exceeds the MAX_FRAME_SIZE. FRAME SIZE ERROR");
    send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
    return -1;
  }
  if(h2s->current_stream.state == STREAM_IDLE){
      if(h2s->header.stream_id < h2s->last_open_stream_id){
        ERROR("Invalid stream id: not bigger than last open. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return -1;
      }
      if(h2s->header.stream_id%2 != h2s->is_server){
        INFO("Incoming stream id: %u", h2s->header.stream_id);
        ERROR("Invalid stream id parity. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return -1;
      }
      else{ // Open a new stream, update last_open and last_peer stream
        h2s->current_stream.stream_id = h2s->header.stream_id;
        h2s->current_stream.state = STREAM_OPEN;
        h2s->last_open_stream_id = h2s->current_stream.stream_id;
        return 0;
      }
  }
  else if(h2s->current_stream.state != STREAM_OPEN &&
          h2s->current_stream.state != STREAM_HALF_CLOSED_LOCAL){
        //stream closed error
        ERROR("Current stream is not open. STREAM CLOSED ERROR");
        send_connection_error(buf_out, HTTP2_STREAM_CLOSED, h2s);
        return -1;
  }
  else if(h2s->header.stream_id != h2s->current_stream.stream_id){
      //protocol error
      ERROR("Stream ids do not match. PROTOCOL ERROR");
      send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
      return -1;
  }
  else{
    return 0;
  }
}

int check_incoming_settings_condition(cbuf_t *buf_out, h2states_t *h2s)
{
  if(h2s->header.stream_id != 0){
      ERROR("Settings frame stream id is not zero. PROTOCOL ERROR");
      send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
      return -1;
  }
  else if(h2s->header.length > read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE)){
    ERROR("Settings payload bigger than allowed. MAX_FRAME_SIZE ERROR");
    send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
    return -1;
  }
  /*Check if ACK is set*/
  if(is_flag_set(h2s->header.flags, SETTINGS_ACK_FLAG)){
      if(h2s->header.length != 0){
          ERROR("Settings payload size is not zero. FRAME SIZE ERROR");
          send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
          return -1;
      }
      else{
          if(h2s->wait_setting_ack){
              h2s->wait_setting_ack = 0;
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

int check_incoming_condition(cbuf_t *buf_out, h2states_t *h2s)
{
    int rc;

    switch (h2s->header.type) {
        case DATA_TYPE: {
            rc = check_incoming_data_condition(buf_out, h2s);
            return rc;
        }
        case HEADERS_TYPE: {
            rc = check_incoming_headers_condition(buf_out, h2s);
            return rc;

        }
        case PRIORITY_TYPE://Priority
            WARN("TODO: Priority Frame. Not implemented yet.");
            return -1;

        case RST_STREAM_TYPE://RST_STREAM
            WARN("TODO: Reset Stream Frame. Not implemented yet.");
            return -1;

        case SETTINGS_TYPE: {
            rc = check_incoming_settings_condition(buf_out, h2s);
            return rc;
        }
        case PUSH_PROMISE_TYPE://Push promise
            WARN("TODO: Push promise frame. Not implemented yet.");
            return -1;

        case PING_TYPE://Ping
            WARN("TODO: Ping frame. Not implemented yet.");
            return -1;

        case GOAWAY_TYPE: {
            rc = check_incoming_goaway_condition(buf_out, h2s);
            return rc;
        }
        case WINDOW_UPDATE_TYPE: {
            return 0;
        }
        case CONTINUATION_TYPE: {
            rc = check_incoming_continuation_condition(buf_out, h2s);
            return rc;
        }
        default: {
            WARN("Error: Type not found");
            return -1;

        }
    }
}

int handle_payload(cbuf_t *buf_out, h2states_t *h2s)
{
    int rc;

    switch (h2s->header.type) {
        case DATA_TYPE: {
            rc = handle_data_payload(buf_out, h2s);
            return rc;
        }
        case HEADERS_TYPE: {
            rc = handle_headers_payload(buf_out, h2s);
            return rc;

        }
        case PRIORITY_TYPE://Priority
            WARN("TODO: Priority Frame. Not implemented yet.");
            return -1;

        case RST_STREAM_TYPE://RST_STREAM
            WARN("TODO: Reset Stream Frame. Not implemented yet.");
            return -1;

        case SETTINGS_TYPE: {
            rc = handle_settings_payload(buf_out, h2s);
            return rc;
        }
        case PUSH_PROMISE_TYPE://Push promise
            WARN("TODO: Push promise frame. Not implemented yet.");
            return -1;

        case PING_TYPE://Ping
            WARN("TODO: Ping frame. Not implemented yet.");
            return -1;

        case GOAWAY_TYPE: {
            rc = handle_goaway_payload(buf_out, h2s);
            return rc;
        }
        case WINDOW_UPDATE_TYPE: {
            return 0;
        }
        case CONTINUATION_TYPE: {
            rc = handle_continuation_payload(buf_out, h2s);
            return rc;
        }
        default: {
            WARN("Error: Type not found");
            return -1;
        }
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
int send_goaway(cbuf_t *buf_out, uint32_t error_code, h2states_t *h2s){//, uint8_t *debug_data_buff, uint8_t debug_size){
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
* Function: send_connection_error
* Send a connection error to endpoint with a specified error code. It implements
* the behaviour suggested in RFC 7540, secion 5.4.1
* Input: -> st: hstates_t pointer where connetion variables are stored
*        -> error_code: error code that will be used to shutdown the connection
* Output: void
*/

void send_connection_error(cbuf_t *buf_out, uint32_t error_code, h2states_t *h2s){
  int rc = send_goaway(buf_out, error_code, h2s);
  if(rc < 0){
    WARN("Error sending GOAWAY frame to endpoint.");
  }
}
