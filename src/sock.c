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
    if((server == NULL) || (server->state != SOCK_LISTENING) || (server->fd <= 0)) {
        errno=EINVAL;
        DEBUG("Error in sock_accept, server must be valid and listening.");
        return -1;
    }

    if(client == NULL) {
        errno=EINVAL;
        DEBUG("Error in sock_accept, NULL client given");
        return -1;
    }

    int clifd = accept(server->fd, NULL, NULL);
    if(clifd < 0) {
        ERROR("Error on accept");
	    return -1;
    }
    client->fd = clifd;
    client->state = SOCK_CONNECTED;
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

int sock_read(sock_t * sock, char * buf, int len, int timeout) {
    if(sock==NULL || (sock->state != SOCK_CONNECTED) || (sock->fd<=0)){
        errno=EINVAL;
        DEBUG("Called sock_read with invalid socket");
        return -1;
    }

    if(buf == NULL){
        errno=EINVAL;
        DEBUG("Error: Called sock_read with null buffer");
        return -1;
    }

    if (timeout < 0) {
        errno = EINVAL;
        DEBUG("Error: Called sock_read with timeout smaller than 0");
        return -1;
    }

    struct timeval time_o;
    time_o.tv_sec = timeout;
    time_o.tv_usec = 0;

    if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time_o, sizeof(time_o)) < 0) {
        DEBUG("Error setting timeout: %s", strerror(errno));
        return -1;
    }

    ssize_t bytes_read = read(sock->fd, buf, len);

    time_o.tv_sec = 0;
    time_o.tv_usec = 0;
    if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time_o, sizeof(time_o)) < 0) {
        DEBUG("Error unsetting timeout: %s", strerror(errno));
        return -1;
     }
    return bytes_read;
}

int sock_write(sock_t * sock, char * buf, int len) {
    if(sock==NULL || sock->state != SOCK_CONNECTED || (sock->fd<=0)) {
        errno=EINVAL;
        DEBUG("Error in sock_write, socket must be valid and connected.");
        return -1;
    }

    if(buf==NULL) {
        errno=EINVAL;
        DEBUG("Error in sock_write, buffer must not be NULL");
        return -1;
    }

    ssize_t bytes_written;
    ssize_t bytes_written_total = 0;
    const char *p = buf;
    while(len>0) {
        if(strcmp(p, "")==0 || p==NULL){
            break;
        }
        bytes_written = write(sock->fd, p, len);
        if(bytes_written < 0) {
            perror("Error writing on socket");
            return -1;
        }

        p += bytes_written;
        len -= bytes_written;
        bytes_written_total += bytes_written;
    }
    return bytes_written;
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
