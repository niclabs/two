#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "cbuf.h"
#include "config.h"
#include "frames.h"
#include "headers.h"



#ifndef HTTP2_V2_H
#define HTTP2_V2_H

callback_t http2_server_init_connection(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
#endif
