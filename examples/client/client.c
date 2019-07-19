#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "http.h"
#include "logging.h"

hstates_t http_client_state;

void cleanup(int signal)
{
    (void)signal;

    INFO("Ctrl-C received. Terminating");
    http_client_disconnect(&http_client_state);
    http_server_destroy(&http_client_state);
}


int main(int argc, char **argv)
{
    if (argc < 3) {
        PRINTF("Usage: %s <addr> <port>\n", argv[0]);
        return 1;
    }

    uint16_t port = atoi(argv[2]);
    if (port < 0) {
        PRINTF("Invalid port given");
        return 1;
    }

    char * addr = argv[1];

    int rc = http_client_connect(&http_client_state, port, addr);

    if (rc < 0) {
        ERROR("in client connect");
    }
    else {
        response_received_type_t rr;
        uint8_t misdatos[128];
        rr.data = misdatos;
        rc = http_get(&http_client_state, "/index", "example.org", "text", &rr);
        if (rc < 0) {
            ERROR("in http_get");
        }
        else {
            INFO("data received: %u \n", rr.size_data);
            for (uint32_t i = 0; i < rr.size_data; i++) {
                printf("%c", (char)rr.data[i]);

            }
            printf("\n");
        }
        http_client_disconnect(&http_client_state);
    }
    return 0;
}
