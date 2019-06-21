#include "server.h"
#include "logging.h"

int send_text(headers_lists_t *headers)
{
    http_set_header(headers, "etag", "Hello world!");
    http_set_header(headers, "expires", "Thu, 23 Jul");
    return 1;
}


void pseudoserver(hstates_t *hs, uint16_t port)
{
    int rc = http_init_server(hs, port);

    if (rc < 0) {
        ERROR("in init server");
    }
    else {

        callback_type_t callback;
        callback.cb = send_text;
        rc = http_set_function_to_path(hs, callback, "index");
        if (rc < 0) {
            ERROR("in http_set_function_to_path");
        }
        else {

            rc = http_start_server(hs);
            if (rc < 0) {
                ERROR("in start server");
            }
        }
    }
}
