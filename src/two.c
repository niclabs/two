/**
 * Basic tcp server using select()
 * Based on https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
 *
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "logging.h"
#include "http.h"

#define PORT (8888)

//sock_t server_sock;
hstates_t http_global_state;

void cleanup(int signal)
{
    (void)signal;

    INFO("Ctrl-C received. Terminating");
    http_client_disconnect(&http_global_state);
    http_server_destroy(&http_global_state);
}

int main(int argc, char **argv){
    if (argc < 2){
        fprintf(stderr, "usage: %s [server|client]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "server") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "usage: %s server <port> \n", argv[0]);
            return 1;
        }

        uint16_t port = atoi(argv[2]);
        http_init_server(&http_global_state, port);
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        if (argc < 3)
        {
            printf("usage: %s client <port> <addr>\n",
                   argv[0]);
            return 1;
        }

        uint16_t port = atoi(argv[2]);
        http_client_connect(&http_global_state, port, argv[3]);
    }
    else
    {
        fprintf(stderr, "usage: %s [server|client]\n", argv[0]);
        return 1;
    }
    return 0;
}
