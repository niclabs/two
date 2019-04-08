#include <assert.h>
#include <string.h>
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
    sock->fd=socket(AF_INET6, SOCK_STREAM, 0);  
    if(sock->fd <0){
	return -1; //TODO specify different types of error.
    }
    else{
	sock->state = OPENED;
	return 0;
    }  
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
    if(accept(server->fd, NULL, NULL)<0){
	return -1; //TODO specify different types of error.
    }
    else{ //TODO include timeout.
	server->state=CONNECTED; 
        client->state=CONNECTED;
	return 0;
    }
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
    int cls = close(sock->fd);
    if(cls<0){
	return -1;//TODO specify different types of error.
    }
    else{
	sock->state=CLOSED;
	return 0;
    } 
}
