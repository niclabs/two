#ifndef HTTP2_SEND_H
#define HTTP2_SEND_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "event.h"
#include "cbuf.h"
#include "http2/structs.h"


int send_data(uint8_t end_stream, h2states_t *h2s);
int send_try_continue_data_sending(h2states_t* h2s);
int send_settings_ack(h2states_t *h2s);
int send_local_settings(h2states_t *h2s);
int send_goaway(uint32_t error_code, h2states_t *h2s);
int send_ping(uint8_t *opaque_data, int8_t ack, h2states_t *h2s);
int send_window_update(uint8_t window_size_increment, h2states_t *h2s);

void send_connection_error(uint32_t error_code, h2states_t *h2s);
void send_stream_error(uint32_t error_code, h2states_t *h2s);
int change_stream_state_end_stream_flag(uint8_t sending, h2states_t *h2s);
int send_headers(uint8_t end_stream, h2states_t *h2s);

/*
 * Function: send_response
 * Given a set of headers and, in some cases, data, generates an http2 message
 * to be send to endpoint. The message is a response, so it must be sent through
 * an existent stream, closing the current stream.
 *
 * @param    buf_out   pointer to hstates_t struct where headers are stored
 * @param    h2s       pointer to hstates_t struct where headers are stored
 *
 * @return   0         Successfully generation and sent of the message
 * @return   -1        There was an error in the process
 */
int send_response(h2states_t *h2s);

#endif
