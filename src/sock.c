#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <assert.h>

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
    // Fail if wrong input given
    assert(sock != NULL);

    // Clean memory
    memset(sock, 0, sizeof(*sock));

    sock->socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock->socket < 0) {
        sock->state = SOCK_CLOSED;
        return -1;
    }

    // Socket options
    int on = 1;

    // Allow socket descriptor to be reuseable
    if (setsockopt(sock->socket, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
        close(sock->socket);
        sock->state = SOCK_CLOSED;

        return -1;
    }

    // Set socket and child sockets to be non blocking
    if (ioctl(sock->socket, FIONBIO, (char *)&on) < 0) {
        close(sock->socket);
        sock->state = SOCK_CLOSED;

        return -1;
    }

    sock->state = SOCK_OPENED;
    return 0;
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

    struct linger sl;
    sl.l_onoff = 1;     /* non-zero value enables linger option in kernel */
    sl.l_linger = 0;    /* timeout interval in seconds */

    // Configure socket to wait for data to send before closing
    setsockopt(sock->socket, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));

    if (close(sock->socket) < 0) {
        return -1;
    }

    sock->socket = -1;
    sock->state = SOCK_CLOSED;
    return 0;
}

unsigned int sock_poll(sock_t *sock)
{
    if (sock == NULL || (sock->state != SOCK_CONNECTED)) {
        errno = EINVAL;
        return -1;
    }

    // peek socket
    ssize_t bytes_available;

    int rc = ioctl(sock->socket, FIONREAD, &bytes_available);

    //bytes_available = recv(sock->socket, NULL, 0, MSG_DONTWAIT | MSG_PEEK | MSG_TRUNC);

    if (bytes_available < 0 || rc < 0) {
        return -1;
    }

    return bytes_available;
}

int sock_read(sock_t *sock, uint8_t *buf, size_t buf_len)
{
    if (sock == NULL || (sock->state != SOCK_CONNECTED)) {
        errno = EINVAL;
        return -1;
    }

    if (buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    // read from socket
    ssize_t bytes_read = recv(sock->socket, buf, buf_len, MSG_DONTWAIT);
    if (bytes_read < 0) {
        return -1;
    }

    return bytes_read;
}

int sock_write(sock_t *sock, uint8_t *buf, size_t buf_len)
{
    if (sock == NULL || sock->state != SOCK_CONNECTED) {
        errno = EINVAL;
        return -1;
    }

    if (buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    /**
     * Note that a successful write() may transfer fewer than count bytes.
     */
    ssize_t bytes_written = send(sock->socket, buf, buf_len, MSG_DONTWAIT);
    if (bytes_written < 0) {
        return -1;
    }

    return bytes_written;
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
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return 0;
        }
        return -1;
    }

    // Only struct values if client is not null
    if (client != NULL) {
        client->socket = clifd;
        client->state = SOCK_CONNECTED;
    }
    return 1;
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
