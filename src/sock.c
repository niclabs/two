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
        errno=11;
        perror("Error creating socket");
        sock->state=SOCK_CLOSED;
	    return -1; 
    }
	sock->state = SOCK_OPENED;
	return 0;
}

int sock_listen(sock_t * server, uint16_t port) {
    if(server->state != SOCK_OPENED){
        errno=22;
        printf("Error in sock_listen, %s, server state must be opened.\n", strerror(errno));
        return -1;
    }
    struct sockaddr_in6 sin6;
    sin6.sin6_family=AF_INET6;
    sin6.sin6_port=htons(port);
    sin6.sin6_addr=in6addr_any; 
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
    if(server->state != SOCK_LISTENING){
        errno=22;
        printf("Error in sock_accept, %s, server state must be listening.\n", strerror(errno));
        return -1;
    }
    if(client == NULL){
        errno=22;
        printf("Error on accept: %s, client must be not NULL.\n", strerror(errno));
        return -1;
    }
    int clifd=accept(server->fd, NULL, NULL);
    if(clifd<0){
        perror("Error on accept");
	    return -1; 
    }
    client->fd=clifd;
	server->state=SOCK_CONNECTED; 
    client->state=SOCK_CONNECTED;
	return 0;
}

int sock_connect(sock_t * client, char * addr, uint16_t port) {
    if(client->state != SOCK_OPENED){
        errno=22;
        printf("Error in sock_connect, %s, client state must be opened.\n", strerror(errno));
        return -1;
    }
    struct sockaddr_in6 sin6;
    struct in6_addr address;
    if(addr==NULL){
        errno=14;
        printf("Error in sock_connect: %s. Address given must not be NULL.\n", strerror(errno));
        return -1;
    }
    int inet_return= inet_pton(AF_INET6, addr, &address);
    if(inet_return<1){
        if(inet_return==0){
            errno=14;
            perror("Error in sock_connect converting IPv6 address to binary");
        }
        if(inet_return==-1){
            perror("Error converting IPv6 address to binary");
        }
        return -1;
    }
    sin6.sin6_port=port;
    sin6.sin6_family=AF_INET6;
    sin6.sin6_addr=address;
    if(connect(client->fd, (struct sockaddr*)&sin6, sizeof(sin6))<0){
        perror("Error on connect");
	    return -1; 
    }
    client->state=SOCK_CONNECTED;
	return 0;
}

int sock_read(sock_t * sock, char * buf, int len, int timeout) {
    if(sock->state != SOCK_CONNECTED){
        errno=22;
        printf("Error in sock_read, %s, sock state must be connected.\n", strerror(errno));
        return -1;
    }
    struct timeval time_o;
    char *p = buf;
    int time_taken=0;
    ssize_t bytes_read;
    clock_t time_now, time_difference;
    time_o.tv_usec = 0;
    while(len>0){
        timeout=timeout-time_taken;
        time_o.tv_sec = timeout;
        if(timeout<0){
            errno=62;
            printf("%s in sock_read function.\n", strerror(errno));
            return -1;
        }
        if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time_o, sizeof(time_o)) < 0) { //what happens when timeout is 0?
            perror("Error setting timeout");
            return -1;
        }
        time_now=clock();
        bytes_read=read(sock->fd, p, len);
        time_difference=clock()-time_now;
        time_taken=(time_difference/CLOCKS_PER_SEC); //in seconds.
        if(bytes_read<0){
            perror("Error reading from socket");
            return -1; 
        }
        p += bytes_read;
        len -= bytes_read;
    }
    time_o.tv_sec = 0;
    time_o.tv_usec = 0;
    if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time_o, sizeof(time_o)) < 0) {
        perror("Error unsetting timeout");
        return -1;
    }
    return 0;
}

int sock_write(sock_t * sock, char * buf, int len) {
    if(sock->state != SOCK_CONNECTED){
        errno=22;
        printf("Error in sock_write, %s, sock state must be connected.\n", strerror(errno));
        return -1;
    }
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
    if(sock->state == SOCK_CLOSED){
        errno=114;
        perror("Error on sock_destroy");
        return -1;
    }
    if(close(sock->fd)<0){
        perror("Error destroying socket");
	    return -1;
    }
	sock->state=SOCK_CLOSED;
	return 0;
}
