#include "server.h"


int send_text(headers_lists_t *headers)
{
    http_set_header(headers, "etag", "Hello world!");
    http_set_header(headers, "expires", "Thu, 23 Jul");
    return 1;
}


void pseudoserver(hstates_t *hs, uint16_t port)
{
    http_init_server(hs, port);

    callback_type_t callback;
    callback.cb = send_text;
    http_set_function_to_path(hs, callback, "index");

    http_start_server(hs);
}
