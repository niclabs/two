#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "sock.h"
#include "logging.h"


int sock_create(sock_t * sock) {
    sock->fd=socket(AF_INET6, SOCK_STREAM, 0);  
    if(sock->fd <0){
	    return -1; //TODO specify different types of error.
    }
    else{
	    sock->state = SOCK_OPENED;
	    return 0;
    }  
}

int sock_listen(sock_t * server, uint16_t port) {
    struct sockaddr_in6 sin6;
    sin6.sin6_family=AF_INET6;
    sin6.sin6_port=htons(port);
    sin6.sin6_addr=in6addr_any; 
    assert(server->state == SOCK_OPENED);
    if(bind(server->fd, (struct sockaddr *)&sin6, sizeof(sin6))<0){
        return -1;
    }
    else if (listen(server->fd, 1)<0){
       return -1;
    }
    else{
	server->state= SOCK_LISTENING;
        return 0;
    }
}

int sock_accept(sock_t * server, sock_t * client) {
    assert(server->state == SOCK_LISTENING);
    int clifd=accept(server->fd, NULL, NULL);
    if(clifd){
	    return -1; //TODO specify different types of error.
    }
    else{ 
	    client->fd=clifd;
	    server->state=SOCK_CONNECTED; 
        client->state=SOCK_CONNECTED;
	    return 0;
    }
}

int sock_connect(sock_t * client, char * addr, uint16_t port) {
    struct sockaddr_in6 sin6;
    struct in6_addr address;
    inet_pton(AF_INET6, addr, &address);
    sin6.sin6_port=port;
    sin6.sin6_family=AF_INET6;
    sin6.sin6_addr=address;
    assert(client->state == SOCK_OPENED);
    if(connect(client->fd, (struct sockaddr*)&sin6, sizeof(sin6))<0){
	    return -1; //TODO specify different types of error.
    }
    else{   
        client->state=SOCK_CONNECTED;
	    return 0;
    }
}

int sock_read(sock_t * sock, char * buf, int len, int timeout) {
    assert(sock->state == SOCK_CONNECTED);
    (void)buf;
    (void)len;
    (void)timeout;

    // TODO

    return -1;
}

int sock_write(sock_t * sock, char * buf, int len) {
    assert(sock->state == SOCK_CONNECTED);
    (void)buf;
    (void)len;

    // TODO

    return -1;
}

int sock_destroy(sock_t * sock) {
    assert(sock->state != SOCK_CLOSED);
    if(close(sock->fd)<0){
	    return -1;//TODO specify different types of error.
    }
    else{
	    sock->state=SOCK_CLOSED;
	    return 0;
    } 
}
