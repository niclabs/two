#include <assert.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include "sock.h"
#include "logging.h"

struct sock {
    int fd;
    enum {
        CLOSED,
        OPENED,
        LISTENING,
        CONNECTED
    } state;
};


int sock_create(sock_t * sock) {
    assert(sock->state == CLOSED);

    // TODO

    return -1;
}

int sock_listen(sock_t * server, uint16_t port) {
    assert(server->state == OPENED);
    (void)port;

    // TODO

    return -1;
}

int sock_accept(sock_t * server, sock_t * client, int timeout) {
    assert(server->state == LISTENING);
    assert(client->state == OPENED);
    (void)timeout;
    // TODO

    return -1;
}

int sock_connect(sock_t * client, char * addr, uint16_t port) {
    assert(client->state == OPENED);
    (void)addr;
    (void)port;
    
    // TODO

    return -1;
}

int sock_read(sock_t * sock, char * buf, int len, int timeout) {
    assert(sock->state == CONNECTED);
    (void)buf;
    (void)len;
    (void)timeout;

    // TODO

    return -1;
}

int sock_write(sock_t * sock, char * buf, int len) {
    assert(sock->state == CONNECTED);
    (void)buf;
    (void)len;

    // TODO

    return -1;
}

int sock_destroy(sock_t * sock) {
    assert(sock->state != CLOSED);

    // TODO

    return -1;
}