#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "cbuf.h"
#include "http2_structs.h"


#ifndef HTTP2_SEND_H
#define HTTP2_SEND_H

int send_data(uint8_t end_stream, cbuf_t *buf_out, h2states_t *h2s);
int send_settings_ack(cbuf_t *buf_out, h2states_t *h2s);
int send_local_settings(cbuf_t *buf_out, h2states_t *h2s);
int send_goaway(uint32_t error_code, cbuf_t *buf_out, h2states_t *h2s);

int send_window_update(uint8_t window_size_increment, cbuf_t *buf_out, h2states_t *h2s);

void send_connection_error(cbuf_t *buf_out, uint32_t error_code, h2states_t *h2s);
int change_stream_state_end_stream_flag(uint8_t sending, cbuf_t *buf_out, h2states_t *h2s);
int send_headers(uint8_t end_stream, cbuf_t *buf_out, h2states_t *h2s);
#endif
