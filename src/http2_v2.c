callback_t h2_server_init_connection(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
callback_t receive_header(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
callback_t receive_payload(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
callback_t receive_payload_wait_settings_ack(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
callback_t receive_payload_goaway(cbuf_t* buf_in, cbuf_t* buf_out, void* state);



/*
*
*
*/
int init_variables_h2s_server(h2states_t * h2s, uint8_t is_server){
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
  h2s->incoming_window.window_size = DEFAULT_INITIAL_WINDOW_SIZE;
  h2s->incoming_window.window_used = 0;
  h2s->outgoing_window.window_size = DEFAULT_INITIAL_WINDOW_SIZE;
  h2s->outgoing_window.window_used = 0;
  hpack_init_states(&(h2s->hpack_states), DEFAULT_HEADER_TABLE_SIZE);
}

callback_t h2_server_init_connection(cbuf_t* buf_in, cbuf_t* buf_out, void* state){
  h2states_t *h2s = (h2states_t*) state;
  int rc = init_variables_h2s_server(h2s, 1);
}
