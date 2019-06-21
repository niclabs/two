/**
 * TCP Socket API abstraction
 * 
 */

#ifndef SOCK_H
#define SOCK_H

#include <stdint.h>

#ifdef WITH_CONTIKI
#include "sys/process.h"

#ifndef SOCK_BUFFER_SIZE
#define SOCK_BUFFER_SIZE 128
#endif

#ifndef SOCK_MAX_SOCKETS
#define SOCK_MAX_SOCKETS UIP_CONNS
#endif

#define PROCESS_SOCK_WAIT(sock, thread) PROCESS_PT_SPAWN(&(sock)->pt, thread)

typedef struct sock_socket * sock_socket_t;
#else
typedef int sock_socket_t;
#endif /* WITH_CONTIKI */


/* Socket states */
typedef enum {
    SOCK_CLOSED,
    SOCK_OPENED,
    SOCK_LISTENING,
    SOCK_CONNECTED
} sock_state_t;

/**
 * Socket typedef
 */
typedef struct {
    sock_socket_t socket;
    int state;
#ifdef WITH_CONTIKI
    struct pt pt;
    uint16_t port;
#endif
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
 * In contiki, the method does not block and returns 0 only if there is a connection already
 * in order for the process to wait for a client, the macro PROCES_SOCk_WAIT_CLIENT must be used
 * before. 
 * 
 * @param   server      pointer to server socket data structure (defined by the implementation)
 * @param   client      pointer to client socket data structure (it will be initialized by the function)
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
 * Read len bytes from the socket into the specified buffer. The method will wait for timeout seconds
 * if timeout is greater than zero
 *
 * In contiki, the method does not block and returns 0 only if there is a connection already
 * in order for the process to wait for new data, the macro PROCES_SOCk_WAIT_DATA must be used
 * before. 
 * 
 * @param   sock    pointer to socket data structure to read from
 * @param   buf     buffer to read the data into
 * @param   len     number of bytes to read
 * @param   timeout timeout in seconds to wait for read before giving up, If set to zero, the method will wait 
 *                  indefinitely or until there is data to read
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
 * @param   sock    pointer to socket data structure
 * 
 * @return  0   is connection was closed succesfully
 * @return  -1  on error (errno will be set appropriately)
 */
int sock_destroy(sock_t * sock);

#ifdef WITH_CONTIKI
/**
 * Wait for a new client connection to arrive
 * given the provided server sock
 *
 * @param sock server sock configured and listening
 */ 
PT_THREAD(sock_wait_client(sock_t * sock));

/**
 * Wait for new data to arrive on the provided client sock
 *
 * @param sock connected client sock
 **/
PT_THREAD(sock_wait_data(sock_t * sock));

/**
 * Process sock macros
 */
#define PROCESS_SOCK_WAIT_CLIENT(sock) PROCESS_SOCK_WAIT(sock, sock_wait_client(sock))
#define PROCESS_SOCK_WAIT_DATA(sock) PROCESS_SOCK_WAIT(sock, sock_wait_data(sock))
#endif


#endif /* SOCK_H */
