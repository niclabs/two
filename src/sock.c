#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "sock.h"
#include "logging.h"
#include "assert.h"

#define BACKLOAD 1

int sock_create(sock_t * sock) {
    sock->fd=socket(AF_INET6, SOCK_STREAM, 0);  
    if(sock->fd <0){
        perror("Error creating socket");
	    return -1; 
    }
	sock->state = SOCK_OPENED;
	return 0;
}

int sock_listen(sock_t * server, u_int16_t port) {
    struct sockaddr_in6 sin6;
    sin6.sin6_family=AF_INET6;
    sin6.sin6_port=htons(port);
    sin6.sin6_addr=in6addr_any; 
    ASSERT(server->state == SOCK_OPENED);
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
    int clifd=accept(server->fd, NULL, NULL);
    ASSERT(server->state == SOCK_LISTENING);
    if(clifd){
        perror("Error on accept");
	    return -1; 
    }
	client->fd=clifd;
	server->state=SOCK_CONNECTED; 
    client->state=SOCK_CONNECTED;
	return 0;
}

int sock_connect(sock_t * client, char * addr, u_int16_t port) {
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
    ASSERT(client->state == SOCK_OPENED);
    if(connect(client->fd, (struct sockaddr*)&sin6, sizeof(sin6))<0){
        perror("Error on connect");
	    return -1; 
    }
    client->state=SOCK_CONNECTED;
	return 0;
}

int sock_read(sock_t * sock, char * buf, int len, int timeout) {
    struct timeval time_o;
    char *p = buf;
    int time_taken=0;
    ssize_t n;
    clock_t t;
    ASSERT(sock->state == SOCK_CONNECTED);
    time_o.tv_usec = 0;
    while(len>0){
        timeout=timeout-time_taken;
        time_o.tv_sec = timeout;
        if(timeout<=0){
            printf("Timeout exceeded in sock_read function.");
            return -1;
        }
        if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time_o, sizeof(time_o)) < 0) {
            perror("Error setting timeout");
            return -1;
        }
        t=clock();
        n=read(sock->fd, p, len);
        t=clock()-t;
        time_taken=(t/CLOCKS_PER_SEC); //in seconds.
        if(n<0){
            perror("Error reading from socket");
            return -1; 
        }
        p += n;
        len -= n;
    }
    time_o.tv_sec = 0;
    time_o.tv_usec = 0;
    if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time_o, sizeof(time_o)) < 0) {
        perror("Error unsetting timeout");
    }
    return 0;
}

int sock_write(sock_t * sock, char * buf, int len) {
    ssize_t n;
    const char *p = buf;
    ASSERT(sock->state == SOCK_CONNECTED);
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
    ASSERT(sock->state != SOCK_CLOSED);
    if(close(sock->fd)<0){
        perror("Error destroying socket");
	    return -1;
    }
	sock->state=SOCK_CLOSED;
	return 0;
}
