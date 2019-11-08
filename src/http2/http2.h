#ifndef HTTP2_H
#define HTTP2_H

#include "cbuf.h"
#include "config.h"

callback_t http2_server_init_connection(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
#endif
