#include <assert.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "sock.h"
#include "logging.h"

#define BACKLOAD 1

//change assertions for errors, create own macro.
int sock_create(sock_t * sock) {
    sock->fd=socket(AF_INET6, SOCK_STREAM, 0);  
    if(sock->fd <0){
        perror("Error creating socket");
	    return -1; 
    }
	sock->state = SOCK_OPENED;
	return 0;
}

int sock_listen(sock_t * server, uint16_t port) {
    struct sockaddr_in6 sin6;
    sin6.sin6_family=AF_INET6;
    sin6.sin6_port=htons(port);
    sin6.sin6_addr=in6addr_any; 
    //assert(server->state == SOCK_OPENED);
    if(bind(server->fd, (struct sockaddr *)&sin6, sizeof(sin6))<0){
        perror("Error on binding");
        return -1;
    }
    if (listen(server->fd, BACKLOAD)<0){
        perror("Error on listening");
        return -1;
    }
	server->state= SOCK_LISTENING;
    return 0;
}

int sock_accept(sock_t * server, sock_t * client) {
    //assert(server->state == SOCK_LISTENING);
    int clifd=accept(server->fd, NULL, NULL);
    if(clifd){
        perror("Error on accept");
	    return -1; 
    }
	client->fd=clifd;
	server->state=SOCK_CONNECTED; 
    client->state=SOCK_CONNECTED;
	return 0;
}

int sock_connect(sock_t * client, char * addr, uint16_t port) {
    struct sockaddr_in6 sin6;
    struct in6_addr address;
    int inet_return= inet_pton(AF_INET6, addr, &address);
    if(inet_return<1){
        if(inet_return==0){
            printf("Error converting IPv6 address to binary: your address does not contain a character string representing a valid network address in the specified family.");
        }
        if(inet_return==-1){
            perror("Error converting IPv6 address to binary");
        }
        return -1;
    }
    sin6.sin6_port=port;
    sin6.sin6_family=AF_INET6;
    sin6.sin6_addr=address;
    //assert(client->state == SOCK_OPENED);
    if(connect(client->fd, (struct sockaddr*)&sin6, sizeof(sin6))<0){
        perror("Error on connect");
	    return -1; 
    }
    client->state=SOCK_CONNECTED;
	return 0;
}

int sock_read(sock_t * sock, char * buf, int len, int timeout) {
    //assert(sock->state == SOCK_CONNECTED);
    (void)timeout; //add timeout to read function.
    ssize_t n;
    const char *p = buf;
    while(len>0){
        n=read(sock->fd, buf, len);
        if(n<0){
            perror("Error reading from socket");
            return -1; 
        }
        p += n;
        len -= n;
    }
    return 0;
}

int sock_write(sock_t * sock, char * buf, int len) {
    //assert(sock->state == SOCK_CONNECTED);
    ssize_t n;
    const char *p = buf;
    while(len>0){
        n=write(sock->fd, buf, len);
        if(n<0){
            perror("Error writing on socket");
            return -1;  
        }   
        p += n;
        len -= n;
    }
    return 0;
}

int sock_destroy(sock_t * sock) {
    //assert(sock->state != SOCK_CLOSED);
    if(close(sock->fd)<0){
        perror("Error destroying socket");
	    return -1;
    }
	sock->state=SOCK_CLOSED;
	return 0;
}
