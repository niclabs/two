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

#define BUF_LEN 256
#define BACKLOAD 1
#define EXIT_STATUS 0

int sock_create(sock_t * sock) {
    sock->fd=socket(AF_INET6, SOCK_STREAM, 0);  
    if(sock->fd <0){
	    return -1; //TODO specify different types of error.
    }
	sock->state = SOCK_OPENED;
	return 0;
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
    else if (listen(server->fd, BACKLOAD)<0){
       return -1;
    }
	server->state= SOCK_LISTENING;
    return 0;
}

int sock_accept(sock_t * server, sock_t * client) {
    assert(server->state == SOCK_LISTENING);
    int clifd=accept(server->fd, NULL, NULL);
    if(clifd){
	    return -1; //TODO specify different types of error.
    }
	client->fd=clifd;
	server->state=SOCK_CONNECTED; 
    client->state=SOCK_CONNECTED;
	return 0;
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
    client->state=SOCK_CONNECTED;
	return 0;
}

int sock_read(sock_t * sock, char * buf, int len, int timeout) {
    assert(sock->state == SOCK_CONNECTED);
    (void)timeout;
   	bzero(buf, BUF_LEN);
   	if((read(sock->fd, buf, len))<0){
        return -1;
 	} //message stored in buf.
    return 0;
}

int sock_write(sock_t * sock, char * buf, int len) {
    assert(sock->state == SOCK_CONNECTED);
    bzero(buf, BUF_LEN);
   	fgets(buf,len,stdin); //store message in buf.
    //from where it is received? FILE *stream?
   	if ((write(sock->fd, buf, len)<0)){
     	   return -1;
    }
    // message (len bytes) written from buf to sock.
    return 0;
}

int sock_destroy(sock_t * sock) {
    assert(sock->state != SOCK_CLOSED);
    if(close(sock->fd)<0){
	    return -1;//TODO specify different types of error.
    }
	sock->state=SOCK_CLOSED;
	return 0;
}
