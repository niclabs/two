#ifndef HTTP2_H
#define HTTP2_H

#include "cbuf.h"
#include "config.h"
#include "event.h"

void http2_server_init_connection(event_sock_t *client, int status);

#endif
