#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "sock.h"
#include "logging.h"

/* From listen(2): The backlog argument defines the maximum length to which the
 * queue of pending connections for sockfd may grow. */
#define SOCK_LISTEN_BACKLOG 1

int sock_create(sock_t *sock)
{
    if (sock == NULL) {
        errno = EINVAL;
        return -1;
    }

    sock->socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock->socket < 0) {
        sock->state = SOCK_CLOSED;
        return -1;
    }

    sock->state = SOCK_OPENED;
    return 0;
}

int sock_listen(sock_t *server, uint16_t port)
{
    if ((server == NULL) || (server->state != SOCK_OPENED)) {
        errno = EINVAL;
        return -1;
    }

    /* Struct sockaddr_in6 needed for binding. Family defined for ipv6. */
    struct sockaddr_in6 sin6;
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(port);
    sin6.sin6_addr = in6addr_any;
    if (bind(server->socket, (struct sockaddr *)&sin6, sizeof(sin6)) < 0) {
        return -1;
    }

    if (listen(server->socket, SOCK_LISTEN_BACKLOG) < 0) {
        return -1;
    }

    server->state = SOCK_LISTENING;
    return 0;
}

int sock_accept(sock_t *server, sock_t *client)
{
    if ((server == NULL) || (server->state != SOCK_LISTENING)) {
        errno = EINVAL;
        return -1;
    }

    int clifd = accept(server->socket, NULL, NULL);
    if (clifd < 0) {
        return -1;
    }

    // Only struct values if client is not null
    if (client != NULL) {
        client->socket = clifd;
        client->state = SOCK_CONNECTED;
    }
    return 0;
}

int sock_connect(sock_t *client, char *addr, uint16_t port)
{
    if (client == NULL || (client->state != SOCK_OPENED)) {
        errno = EINVAL;
        return -1;
    }

    if (addr == NULL) {
        errno = EINVAL;
        return -1;
    }

    // convert address string to a socket address
    struct in6_addr address;
    int pton_res = inet_pton(AF_INET6, addr, &address);

    // string is not valid INET6 address
    if (pton_res == 0) {
        errno = EINVAL;
        ERROR("'%s' is not a valid IPv6 address", addr);
        return -1;
    }

    // AF_INET6 is not supported
    if (pton_res < 0) {
        ERROR("The system does not support IPv6 addresses");
        return -1;
    }

    struct sockaddr_in6 sin6;
    sin6.sin6_port = htons(port);
    sin6.sin6_family = AF_INET6;
    sin6.sin6_addr = address;
    if (connect(client->socket, (struct sockaddr *)&sin6, sizeof(sin6)) < 0) {
        ERROR("Failed to connect to [%s]:%d", addr, port);
        return -1;
    }

    client->state = SOCK_CONNECTED;
    return 0;
}

int sock_read(sock_t *sock, char *buf, int len, int timeout)
{
    if (sock == NULL || (sock->state != SOCK_CONNECTED)) {
        errno = EINVAL;
        return -1;
    }

    if (buf == NULL) {
        errno = EINVAL;
        return -1;
    }


    // set timeout
    if (timeout > 0) {
        // use select to wait for sock to have reading data
        fd_set read_fds;
        FD_ZERO(&read_fds);             // prepare fd_set
        FD_SET(sock->socket, &read_fds);    // add sock->socket to fd_set

        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        int res = select(sock->socket + 1, &read_fds, NULL, NULL, &tv);
        if (res == -1) {
            return -1;
        }
        else if (res != 1) { // timeout reached
            errno = ETIME;
            return -1;
        }
    }

    // read from socket
    return read(sock->socket, buf, len);
}

int sock_write(sock_t *sock, char *buf, int len)
{
    if (sock == NULL || sock->state != SOCK_CONNECTED) {
        errno = EINVAL;
        return -1;
    }

    if (buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    ssize_t bytes_written;
    ssize_t bytes_written_total = 0;
    const char *p = buf;

    /**
     * Note that a successful write() may transfer fewer than count bytes.
     * Is expected that when all bytes are transfered, there's no length
     * left of the message and/or there's no information left in the
     * buffer.
     */
    while (len > 0) {
        bytes_written = write(sock->socket, p, len);
        if (bytes_written < 0) {
            return -1;
        }
        p += bytes_written;
        len -= bytes_written;
        bytes_written_total += bytes_written;
    }
    return bytes_written_total;
}

int sock_destroy(sock_t *sock)
{
    if (sock == NULL || sock->state != SOCK_CONNECTED) {
        errno = EINVAL;
        return -1;
    }

    if (sock->state == SOCK_CLOSED) {
        errno = EBADF;
        return -1;
    }

    if (close(sock->socket) < 0) {
        return -1;
    }

    sock->socket = -1;
    sock->state = SOCK_CLOSED;
    return 0;
}
