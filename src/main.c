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
#include "sock.h"

#define PORT (8888)

sock_t server_sock;

void cleanup(int signal)
{
    (void)signal;

    INFO("Ctrl-C received. Terminating");
    sock_destroy(&server_sock);
    exit(0);
}

void client_start(char * addr, char * port_str, char * endpoint) {
    uint16_t port = atoi(port_str);
    if (port == 0)
    {
        ERROR("Invalid port specified");
        return;
    }

    sock_t client_sock;
    sock_create(&client_sock);
    if (sock_connect(&client_sock, addr, port) == 0) {
        sock_write(&client_sock, endpoint, strlen(endpoint));
    }
    sock_destroy(&client_sock);
}

void server_start(char * port_str) {
    uint16_t port = atoi(port_str);
    if (port == 0)
    {
        ERROR("Invalid port specified");
        return;
    }

    sock_create(&server_sock);
    sock_listen(&server_sock, port);

    // Trap Ctrl-C
    signal(SIGINT, cleanup);

    sock_t client_sock;
    while (sock_accept(&server_sock, &client_sock) >= 0) {
        INFO("Client connected");
        char buf[256];
        while (sock_read(&client_sock, buf, 256, 0) > 0) {
            INFO("Received %s", buf);
        }
        sock_destroy(&client_sock);
    }

    // Cleanup after listen terminates
    sock_destroy(&server_sock);
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
