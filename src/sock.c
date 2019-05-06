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

//use logging.h to print errors.
int sock_create(sock_t * sock) {
    if(sock==NULL){
        errno=EINVAL;
        perror("Error creating socket");
        return -1;
    }
    sock->fd=socket(AF_INET6, SOCK_STREAM, 0);  
    if(sock->fd <0){
        errno=EAGAIN;
        perror("Error creating socket");
        sock->state=SOCK_CLOSED;
	    return -1; 
    }
	sock->state = SOCK_OPENED;
	return 0;
}

int sock_listen(sock_t * server, uint16_t port) {
    if((server==NULL) || (server->state != SOCK_OPENED) || (server->fd<=0)){
        errno=EINVAL;
        printf("Error in sock_listen, %s, server must valid and be opened.\n", strerror(errno));
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
    if((server==NULL) || (server->state != SOCK_LISTENING) || (server->fd<=0)){
        errno=EINVAL;
        printf("Error in sock_accept, %s, server must be valid and listening.\n", strerror(errno));
        return -1;
    }
    if(client == NULL){
        errno=EINVAL;
        printf("Error on accept: %s, client must be valid and CONNECTED.\n", strerror(errno));
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
    if(client==NULL || (client->state != SOCK_OPENED) || (client->fd<=0)){
        errno=EINVAL;
        printf("Error in sock_connect, %s, client must be valid.\n", strerror(errno));
        return -1;
    }
    struct sockaddr_in6 sin6;
    struct in6_addr address;
    if(addr==NULL){
        errno=EFAULT;
        printf("Error in sock_connect: %s. Address given must not be NULL.\n", strerror(errno));
        return -1;
    }
    int inet_return= inet_pton(AF_INET6, addr, &address);
    if(inet_return<1){
        if(inet_return==0){
            errno=EFAULT;
            perror("Error in sock_connect converting IPv6 address to binary");
        }
        if(inet_return==-1){
            perror("Error converting IPv6 address to binary");
        }
        return -1;
    }
    sin6.sin6_port=htons(port);
    sin6.sin6_family=AF_INET6;
    sin6.sin6_addr=address;
    int res=connect(client->fd, (struct sockaddr*)&sin6, sizeof(sin6));
    if(res<0){
        perror("Error on connect");
	    return -1; 
    }
    client->state=SOCK_CONNECTED;
	return 0;
}
//timeout of 0 should mean there is no timeout. 
int sock_read(sock_t * sock, char * buf, int len, int timeout) {
    if(sock==NULL || (sock->state != SOCK_CONNECTED) || (sock->fd<=0)){
        errno=EINVAL;
        printf("Error in sock_read, %s, socket must be valid and CONNECTED.\n", strerror(errno));
        return -1;
    }
    if(buf==NULL){
        errno=EINVAL;
        perror("Error in sock_read, buffer must not be NULL");
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
            errno=ETIME;
            printf("%s in sock_read function.\n", strerror(errno));
            return -1;
        }
        if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time_o, sizeof(time_o)) < 0) { //what happens when timeout is 0?
            perror("Error setting timeout");
            return -1;
        }
        time_now=clock();
        bytes_read=read(sock->fd, p, len);
        //fprintf(stderr,"Bytes read were %d", bytes_read);//MUST BE ERASED IN FUTURE
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
    if(sock==NULL || sock->state != SOCK_CONNECTED || (sock->fd<=0)){
        errno=EINVAL;
        printf("Error in sock_write, %s, socket must be valid and connected.\n", strerror(errno));
        return -1;
    }
    if(buf==NULL){
        errno=EINVAL;
        perror("Error in sock_write, buffer must not be NULL");
        return -1;
    }
    ssize_t bytes_written;
    const char *p = buf;
    while(len>0){
        bytes_written=write(sock->fd, p, len);
        //fprintf(stderr,"Bytes written were %d", bytes_written);//MUST BE ERASED IN FUTURE
        if(bytes_written<0){
            perror("Error writing on socket");
            return -1;  
        }   
        p += bytes_written;
        len -= bytes_written;
    }
    return 0;
}

int sock_destroy(sock_t * sock) {
    if(sock==NULL || sock->fd<=0){
        errno=EINVAL;
        perror("Error on socket_destroy");
        return -1;
    }
    if(sock->state == SOCK_CLOSED){
        errno=EALREADY;
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
