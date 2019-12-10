#ifndef HTTP2_CHECK_H
#define HTTP2_CHECK_H

#include "http2/structs.h"
#include "cbuf.h"

int check_incoming_data_condition(h2states_t *h2s);
int check_incoming_headers_condition(h2states_t *h2s);
int check_incoming_settings_condition(h2states_t *h2s);
int check_incoming_goaway_condition(h2states_t *h2s);
int check_incoming_window_update_condition(h2states_t *h2s);
int check_incoming_continuation_condition(h2states_t *h2s);
int check_incoming_ping_condition(h2states_t *h2s);
#endif
