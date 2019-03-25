/**
 * Basic tcp server using select()
 * Based on https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
 * 
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT (8888)
#define MAX_BUF_SIZE (128)
#define MAX_CLIENTS (1)

int make_socket(uint16_t port)
{
    int sock = -1, option = 1;
    struct sockaddr_in6 addr;

    /* Create the socket. */
    sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Give the socket a addr. */
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    memset(&addr.sin6_addr, 0, sizeof(addr.sin6_addr));

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /* avoid waiting port close time to use server immediately */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option)) < 0)
    {
        perror("setsockopt");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}

int read_from_client(int fd)
{
    char buffer[MAX_BUF_SIZE];
    int nbytes;

    nbytes = read(fd, buffer, MAX_BUF_SIZE);
    if (nbytes < 0)
    {
        /* Read error. */
        perror("read");
        exit(EXIT_FAILURE);
    }
    else if (nbytes == 0)
        /* End-of-file. */
        return -1;
    else
    {
        /* Data read. */
        fprintf(stderr, "Server: got message: '%.*s'\n", nbytes - 1, buffer);
        return 0;
    }
}

int main(void)
{
    int sock;
    fd_set active_fd_set, read_fd_set;
    int i;
    char client_addr_str[INET6_ADDRSTRLEN];
    struct sockaddr_in6 client_addr;
    socklen_t size;
    int clients = 0;

    /* Create the socket and set it up to accept connections. */
    sock = make_socket(PORT);
    if (listen(sock, MAX_CLIENTS) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* Initialize the set of active sockets. */
    FD_ZERO(&active_fd_set);
    FD_SET(sock, &active_fd_set);

    while (1)
    {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i)
            if (FD_ISSET(i, &read_fd_set))
            {
                if (i == sock)
                {
                    /* Connection request on original socket. */
                    int new;
                    size = sizeof(client_addr);
                    new = accept(sock,
                                 (struct sockaddr *)&client_addr,
                                 &size);
                    if (new < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    };

                    inet_ntop(AF_INET6, &client_addr.sin6_addr, client_addr_str, sizeof(client_addr_str));
                    fprintf(stderr,
                            "Server: connect from host %s, port %u. ",
                            client_addr_str,
                            client_addr.sin6_port);
                    if (clients < MAX_CLIENTS)
                    {
                        FD_SET(new, &active_fd_set);
                        clients++;
                        fprintf(stderr, "Acepted\n");
                    }
                    else
                    {
                        // Reject new connection
                        close(new);
                        fprintf(stderr, "Rejected\n");
                    }
                }
                else
                {
                    /* Data arriving on an already-connected socket. */
                    if (read_from_client(i) < 0)
                    {
                        close(i);
                        FD_CLR(i, &active_fd_set);
                        clients--;
                    }
                }
            }
    }
}