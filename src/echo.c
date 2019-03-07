#include <stdint.h>

#include "common.h"
#include "net/af.h"
#include "net/ipv6/addr.h"
#include "net/sock/tcp.h"

#ifdef MODULE_SOCK_TCP
static char buf[SOCK_BUF_SIZE];
static sock_tcp_queue_t server_queue;
static sock_tcp_t server_sock, client_sock;

static int _serve(uint16_t port)
{
    sock_tcp_ep_t server_addr = SOCK_IPV6_EP_ANY;

    int res;
    server_addr.port = port;

    if ((res = sock_tcp_listen(&server_queue, &server_addr, &server_sock, SOCK_QUEUE_LEN, 0)) < 0)
    {
        printf("Unable to open TCP server on port %" PRIu16 " (error code %d)\n",
               server_addr.port, -res);
        return res;
    }
    printf("Success: started TCP server on port %" PRIu16 "\n",
           server_addr.port);

    while (1)
    {
        sock_tcp_t *sock = NULL;
        char client_addr[IPV6_ADDR_MAX_STR_LEN];
        unsigned client_port;

        if ((res = sock_tcp_accept(&server_queue, &sock, SOCK_NO_TIMEOUT)) < 0)
        {
            puts("Error on TCP accept");
            continue;
        }
        else
        {
            sock_tcp_ep_t client;

            sock_tcp_get_remote(sock, &client);
            ipv6_addr_to_str(client_addr, (ipv6_addr_t *)&client.addr.ipv6,
                             sizeof(client_addr));
            client_port = client.port;
            printf("TCP client [%s]:%u connected\n",
                   client_addr, client_port);
        }

        /* we don't use timeouts so all errors should be related to a lost
         * connection */
        while ((res = sock_tcp_read(sock, buf, sizeof(buf),
                                    SOCK_NO_TIMEOUT)) >= 0)
        {
            printf("Received TCP data from client [%s]:%u:\n",
                   client_addr, client_port);
            if (res > 0)
            {
                printf("Received: \"");
                for (int i = 0; i < res; i++)
                {
                    printf("%c", buf[i]);
                }
                puts("\"");

                // Write data back
                if (sock_tcp_write(sock, &buf, res) < 0)
                {
                    puts("Errored on write, finished server loop");
                    break;
                }
            }
            else
            {
                puts("Disconnected");
                break;
            }
        }
        printf("TCP connection to [%s]:%u reset, starting to accept again\n",
               client_addr, client_port);
    }
    sock_tcp_stop_listen(&server_queue);

    return 0;
}

int _client(char *addr, uint16_t port, char *msg)
{
    sock_tcp_ep_t dst = SOCK_IPV6_EP_ANY;
    int res;
    (void)msg;

    /* parse destination address */
    if (ipv6_addr_from_str((ipv6_addr_t *)&dst.addr.ipv6, addr) == NULL)
    {
        puts("Error: unable to parse destination address");
        return 1;
    }
    dst.port = port;
    if ((res = sock_tcp_connect(&client_sock, &dst, 0, 0)) < 0)
    {
        printf("Unable to connect to TCP server on port %" PRIu16 " (error code %d)\n",
               dst.port, -res);
        return res;
    }

    if (sock_tcp_write(&client_sock, msg, strlen(msg)) < 0)
    {
        puts("Could not send data");
    }
    else
    {
        int res;
        if ((res = sock_tcp_read(&client_sock, &buf, sizeof(buf),
                                 SOCK_NO_TIMEOUT)) < 0)
        {
            puts("Disconnected");
        }

        for (int i = 0; i < res; i++)
        {
            printf("%c", buf[i]);
        }
        puts("");
    }
    sock_tcp_disconnect(&client_sock);

    return 0;
}

int echo_server(int argc, char **argv)
{
    uint16_t port;

    if (argc < 2)
    {
        printf("usage: %s <port>\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
    return _serve(port);
}

int echo_client(int argc, char **argv)
{
    uint16_t port;

    if (argc < 4)
    {
        printf("usage: %s <addr> <port> <msg>\n", argv[0]);
        return 1;
    }

    port = atoi(argv[2]);
    return _client(argv[1], port, argv[3]);
}

#endif
