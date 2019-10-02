#include "http2_v2.h"
#include <string.h>

callback_t h2_server_init_connection(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
callback_t receive_header(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
callback_t receive_payload(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
callback_t receive_payload_wait_settings_ack(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
callback_t receive_payload_goaway(cbuf_t* buf_in, cbuf_t* buf_out, void* state);

/*
*
*
*/
int init_variables_h2s(h2states_t * h2s, uint8_t is_server){
  h2s->remote_settings[0] = h2s->local_settings[0] = DEFAULT_HEADER_TABLE_SIZE;
  h2s->remote_settings[1] = h2s->local_settings[1] = DEFAULT_ENABLE_PUSH;
  h2s->remote_settings[2] = h2s->local_settings[2] = DEFAULT_MAX_CONCURRENT_STREAMS;
  h2s->remote_settings[3] = h2s->local_settings[3] = DEFAULT_INITIAL_WINDOW_SIZE;
  h2s->remote_settings[4] = h2s->local_settings[4] = DEFAULT_MAX_FRAME_SIZE;
  h2s->remote_settings[5] = h2s->local_settings[5] = DEFAULT_MAX_HEADER_LIST_SIZE;
  h2s->current_stream.stream_id = is_server ? 2 : 3;
  h2s->current_stream.state = STREAM_IDLE;
  h2s->last_open_stream_id = 1;
  h2s->header_block_fragments_pointer = 0;
  h2s->incoming_window.window_size = DEFAULT_INITIAL_WINDOW_SIZE;
  h2s->incoming_window.window_used = 0;
  h2s->outgoing_window.window_size = DEFAULT_INITIAL_WINDOW_SIZE;
  h2s->outgoing_window.window_used = 0;
  hpack_init_states(&(h2s->hpack_states), DEFAULT_HEADER_TABLE_SIZE);
}

callback_t h2_server_init_connection(cbuf_t* buf_in, cbuf_t* buf_out, void* state){
  if(cbuf_len(buf_in) < 24){
    callback_t ret = {h2_server_init_connection};
    return ret;
  }
  int rc;
  uint8_t preface_buff[25];
  char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
  rc = cbuf_pop(buf_in, preface_buff, 24);
  if(rc != 24){
    callback_t ret_null = {NULL, NULL};
    return ret_null;
  }
  preface_buff[24] = '\0';
  if(strcmp(preface, (char*)preface_buff) != 0){
      ERROR("Error in preface receiving");
      send_connection_error(st, HTTP2_PROTOCOL_ERROR);
      callback_t ret_null = {NULL, NULL};
      return ret_null;
  }
  h2states_t *h2s = (h2states_t*) state;
  rc = init_variables_h2s(h2s, 1);
  callback_t ret_null = {NULL, NULL};
  return ret_null;
}
