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
#include "server.h"
#include "client.h"

#define PORT (8888)

void cleanup(int signal)
{
    (void)signal;

    INFO("Ctrl-C received. Terminating.\n");
}

void client_start(char * addr, char * port_str, char * endpoint) {
    uint16_t port = atoi(port_str);
    if (port == 0)
    {
        ERROR("Invalid port specified\n");
        return;
    }

    client_t * client = client_create(addr, port);
    client_connect(client);
    client_request(client, endpoint);
}

void server_start(char * port_str) {
    uint16_t port = atoi(port_str);
    if (port == 0)
    {
        ERROR("Invalid port specified\n");
        return;
    }

    /* Create the socket and set it up to accept connections. */
    server_t * server = server_create(port);

    // Trap Ctrl-C
    signal(SIGINT, cleanup);

    // Start listening for connections
    server_listen(server);

    // Cleanup after listen terminates
    server_destroy(server);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s [server|get]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "server") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "usage: %s server <port>\n", argv[0]);
            return 1;
        }
        server_start(argv[2]);
    }
    else if (strcmp(argv[1], "get") == 0)
    {
        if (argc < 5)
        {
            printf("usage: %s get <addr> <port> <endpoint>]\n",
                   argv[0]);
            return 1;
        }

        client_start(argv[2], argv[3], argv[4]);
    }
    else
    {
        fprintf(stderr, "usage: %s [server|get]\n", argv[0]);
        return 1;
    }
    return 0;
}
