#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "http.h"
#include "logging.h"

hstates_t http_server_state;

void cleanup(int signal)
{
    (void)signal;

    INFO("Ctrl-C received. Terminating");
    http_client_disconnect(&http_server_state);
    http_server_destroy(&http_server_state);
}

int send_text(headers_data_lists_t *headers_and_data)
{
    http_set_header(headers_and_data, "etag", "Hello world!");
    http_set_header(headers_and_data, "expires", "Thu, 23 Jul");
    http_set_data(headers_and_data, (uint8_t *)"Hello world!",12 );
    return 1;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        PRINTF("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    uint16_t port = atoi(argv[1]);
    if (port < 0) {
        PRINTF("Invalid port given");
        return 1;
    }

    int rc = http_server_create(&http_server_state, port);

    if (rc < 0) {
        ERROR("in init server");
    }
    else {
        callback_type_t callback;
        callback.cb = send_text;
        rc = http_set_resource_cb(&http_server_state, callback, "index");
        if (rc < 0) {
            ERROR("in http_set_resource");
        }
        else {

            rc = http_server_start(&http_server_state);
            if (rc < 0) {
                ERROR("in start server");
            }
        }
    }
}
