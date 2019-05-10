/**
 * TCP Socket API abstraction
 * 
 */

#ifndef SOCK_H
#define SOCK_H

#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


/**
 * Socket typedef
 */
typedef struct {
    int fd;
    enum {
        SOCK_CLOSED,
        SOCK_OPENED,
        SOCK_LISTENING,
        SOCK_CONNECTED
    } state;
    struct sockaddr_in6 sin6;
} sock_t;

/**
 * Initialize a new socket to act as either a server or a client
 * 
 * @param sock pointer to socket data structure (defined by the implementation)
 * 
 * @return  0   if socket was created succesfully
 * @return -1   on error (errno will be set appropriately)
 */
int sock_create(sock_t * sock);

/**
 * Configure provided socket to act as a server in the specified port
 * 
 * @param   server  pointer to socket data structure (defined by the implementation)
 * @param   port    port to listen for connections
 * 
 * @return  0   if setting socket to act as a server succeeds
 * @return -1   on error (errno will be set appropriately)
 */
int sock_listen(sock_t * server, uint16_t port);

/**
 * Wait for connections on server socket. Server socket must be set to listen by calling
 * sock_listen() previously, otherwise an error will be returned.
 * 
 * @param   server      pointer to server socket data structure (defined by the implementation)
 * @param   client      pointer to client socket, socket must be initialized with sock_create() 
 *                      otherwise error will be returned. 
 * @param   timeout     timeout in seconds to wait for a new connection before giving up. 
 *                      if set to zero the method will wait indefinetely
 * 
 * @return   0      if a new client was successfully accepted
 * @return  -1      on error (errno will be set appropriately)
 */
int sock_accept(sock_t * server, sock_t * client);

/**
 * Connect the client socket to the specified address and port. The socket must be initialized with
 * sock_create() previously
 * 
 * @param   client  pointer to client socket data structure
 * @param   address remote address to connect the socket to
 * @param   port    remote port to connect to
 * 
 * @return   0      if a new connection was successfully established
 * @return  -1      on error (errno will be set appropriately)
 */
int sock_connect(sock_t * client, char * addr, uint16_t port);

/**
 * Read len bytes from the socket into the specified buffer. The method will wait for timeout seconds or until
 * all bytes are read
 * 
 * @param   sock    pointer to socket data structure to read from
 * @param   buf     buffer to read the data into
 * @param   len     number of bytes to read
 * @param   timeout timeout in seconds to wait for read before giving up, If set to zero, the method will wait 
 *                  indefinitely
 * 
 * @return   >0     number of bytes read
 * @return   0      if connection was closed on the other side
 * @return  -1      on error (errno will be set appropriately)
 */
int sock_read(sock_t * sock, char * buf, int len, int timeout);

/**
 * Write len bytes from the buffer into the socket. The method will attempt to write all bytes unless an error 
 * ocurrs
 * 
 * @param   sock    pointer to socket data structure to read from
 * @param   buf     buffer to read the data into
 * @param   len     number of bytes to read
 * 
 * @return >0   number of bytes written
 * @return 0    if connection was closed on the other side
 * @return -1   on error (errno will be set appropriately)
 */
int sock_write(sock_t * sock, char * buf, int len);

/**
 * Close the connection and destroy the socket
 * 
 * @param   sock    pointer to socket data structure to read from
 * 
 * @return  0   is connection was closed succesfully
 * @return  -1  on error (errno will be set appropriately)
 */
int sock_destroy(sock_t * sock);


#endif /* SOCK_H */