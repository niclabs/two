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

    PRINTF("Ctrl-C received. Terminating");
    http_client_disconnect(&http_server_state);
    http_server_destroy(&http_server_state);
}

int send_text(char * method, char * uri, uint8_t * response, int maxlen)
{
    (void)method;
    (void)uri;
    (void)maxlen;
    memcpy(response, "hello world!!!!", 16);
    return 16;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        PRINTF("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port < 0) {
        PRINTF("Invalid port given");
        return 1;
    }
/*
    int rc = http_server_create(&http_server_state, port);

    if (rc < 0) {
        ERROR("in init server");
    }
    else {
        rc = http_server_register_resource(&http_server_state, "GET", "/index", &send_text);
        if (rc < 0) {
            ERROR("in http_register_resource");
        }
        else {

            rc = http_server_start(&http_server_state);
            if (rc < 0) {
                ERROR("in start server");
            }
        }
    }
    */
}
