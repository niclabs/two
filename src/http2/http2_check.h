#include "cbuf.h"
#include "http2_structs.h"


#ifndef HTTP2_V2_H
#define HTTP2_V2_H

int check_incoming_data_condition(cbuf_t *buf_out, h2states_t *h2s);
int check_incoming_headers_condition(cbuf_t *buf_out, h2states_t *h2s);
int check_incoming_settings_condition(cbuf_t *buf_out, h2states_t *h2s);
int check_incoming_goaway_condition(cbuf_t *buf_out, h2states_t *h2s);
int check_incoming_continuation_condition(cbuf_t *buf_out, h2states_t *h2s);
#endif
