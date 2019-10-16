#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "cbuf.h"
#include "http2_structs.h"


#ifndef HTTP2_SEND_H
#define HTTP2_SEND_H

int send_goaway(uint32_t error_code, cbuf_t *buf_out, h2states_t *h2s);
void send_connection_error(cbuf_t *buf_out, uint32_t error_code, h2states_t *h2s);
#endif
