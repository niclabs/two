#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "http_old.h"
#include "logging.h"

hstates_t http_client_state;

void cleanup(int signal)
{
    (void)signal;

    PRINTF("Ctrl-C received. Terminating");
    http_client_disconnect(&http_client_state);
    http_server_destroy(&http_client_state);
}


int main(int argc, char **argv)
{
    if (argc < 3) {
        PRINTF("Usage: %s <addr> <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[2]);
    if (port < 0) {
        PRINTF("%s is not a valid port", argv[2]);
        return 1;
    }

    char *addr = argv[1];

    // Connect to remote server
    int rc = http_client_connect(&http_client_state, addr, port);

    if (rc < 0) {
        return 1;
    }

    size_t response_size = 128;
    uint8_t response[response_size];
    int status = http_get(&http_client_state, "/index", response, &response_size);
    if (rc < 0) {
        return 1;
    }
    else if (status == 200) {
        PRINTF("Received %d bytes", (int)response_size);
        for (uint32_t i = 0; i < response_size; i++) {
            PRINTF("%c", (char)response[i]);
        }
        PRINTF("\n");
    }
    else {
        ERROR("Server responded with status %d", status);
    }
    http_client_disconnect(&http_client_state);
    return 0;
}
