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

    sock->fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock->fd < 0) {
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
    if (bind(server->fd, (struct sockaddr *)&sin6, sizeof(sin6)) < 0) {
        return -1;
    }
    
    if (listen(server->fd, SOCK_LISTEN_BACKLOG) < 0) {
        return -1;
    }
    
    server->state = SOCK_LISTENING;
    return 0;
}

int sock_accept(sock_t *server, sock_t *client)
{
    if ((server == NULL) || (server->state != SOCK_LISTENING) || (server->fd < 0)) {
        errno = EINVAL;
        return -1;
    }

    if (client == NULL) {
        errno = EINVAL;
        return -1;
    }

    int clifd = accept(server->fd, NULL, NULL);
    if (clifd < 0) {
        return -1;
    }
    client->fd = clifd;
    client->state = SOCK_CONNECTED;
    server->state = SOCK_CONNECTED;
    return 0;
}

int sock_connect(sock_t *client, char *addr, uint16_t port)
{
    if (client == NULL || (client->state != SOCK_OPENED)) {
        errno = EINVAL;
        DEBUG("Error in sock_connect, client must be valid and opened");
        return -1;
    }
    /*Struct sockaddr_in6 is used to store information about client in connect function.*/
    struct sockaddr_in6 sin6;
    struct in6_addr address;
    if (addr == NULL) {
        errno = EFAULT;
        DEBUG("Error in sock_connect, NULL address given");
        return -1;
    }
    int inet_return = inet_pton(AF_INET6, addr, &address);
    if (inet_return < 1) {
        if (inet_return == 0) {
            errno = EFAULT;
            ERROR("Error converting IPv6 address to binary");
        }
        if (inet_return == -1) {
            ERROR("Error converting IPv6 address to binary");
        }
        return -1;
    }
    sin6.sin6_port = htons(port);
    sin6.sin6_family = AF_INET6;
    sin6.sin6_addr = address;
    int res = connect(client->fd, (struct sockaddr *)&sin6, sizeof(sin6));
    if (res < 0) {
        ERROR("Error on connect");
        return -1;
    }
    client->state = SOCK_CONNECTED;
    return 0;
}

int sock_read(sock_t *sock, char *buf, int len, int timeout)
{
    if (sock == NULL || (sock->state != SOCK_CONNECTED) || ((sock->fd) < 0)) {
        errno = EINVAL;
        DEBUG("Called sock_read with invalid socket");
        return -1;
    }

    if (buf == NULL) {
        errno = EINVAL;
        DEBUG("Error: Called sock_read with null buffer");
        return -1;
    }

    if (timeout < 0) {
        errno = EINVAL;
        DEBUG("Error: Called sock_read with timeout smaller than 0");
        return -1;
    }

    /*Code below is needed to set timeout to function*/
    fd_set set;
    struct timeval tv;

    FD_ZERO(&set);
    FD_SET(sock->fd, &set);

    tv.tv_sec = 0;
    tv.tv_usec = timeout;

    int rv = select((sock->fd) + 1, &set, NULL, NULL, &tv);

    if (rv < 0) {
        if (rv == 0) {
            errno = ETIMEDOUT;
            DEBUG("Timeout reached");
        }
        else {
            ERROR("Error setting timeout");
        }
        return -1;
    }
    /*----------------------------------------------*/
    ssize_t bytes_read = read(sock->fd, buf, len);
    if (bytes_read < 0) {
        ERROR("Error reading from socket");
        return -1;
    }
    return bytes_read;
}

int sock_write(sock_t *sock, char *buf, int len)
{
    if (sock == NULL || sock->state != SOCK_CONNECTED || (sock->fd < 0)) {
        errno = EINVAL;
        DEBUG("Error in sock_write, socket must be valid and connected.");
        return -1;
    }

    if (buf == NULL) {
        errno = EINVAL;
        DEBUG("Error in sock_write, buffer must not be NULL");
        return -1;
    }
    ssize_t bytes_written;
    ssize_t bytes_written_total = 0;
    const char *p = buf;
    /*Note that a successful write() may transfer fewer than count bytes. Is expected that when
       all bytes are transfered, there's no length left of the message and/or there's no information
       left in the buffer.*/
    while (len > 0) {
        if (p == NULL) {
            break;
        }
        bytes_written = write(sock->fd, p, len);
        if (bytes_written < 0) {
            ERROR("Error writing on socket");
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
    if (sock == NULL || sock->fd < 0) {
        errno = EINVAL;
        DEBUG("Error on socket_destroy, NULL socket given");
        return -1;
    }
    if (sock->state == SOCK_CLOSED) {
        errno = EALREADY;
        DEBUG("Error on sock_destroy, socket already closed");
        return -1;
    }
    if (close(sock->fd) < 0) {
        ERROR("Error destroying socket");
        return -1;
    }
    sock->state = SOCK_CLOSED;
    return 0;
}
