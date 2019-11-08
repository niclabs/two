/**
 * TCP Socket API abstraction
 * 
 */

#ifndef SOCK_H
#define SOCK_H

#include <stdlib.h>
#include <stdint.h>

#ifdef WITH_CONTIKI
#include "sys/process.h"

#ifdef SOCK_CONF_BUFFER_SIZE
#define SOCK_BUFFER_SIZE (SOCK_CONF_BUFFER_SIZE)
#else 
#define SOCK_BUFFER_SIZE 128
#endif

#ifdef SOCK_CONF_MAX_SOCKETS
#define SOCK_MAX_SOCKETS (SOCK_CONF_MAX_SOCKETS)
#else
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
 * Initialize socket memory before using. This function is already called by sock_create
 * but it is recommended to be called before for client socket given to sock_accept()
 *
 * @param sock pointer to socket data structure (defined by the implementation)
 *
 * @return 0 if socket was initialized correctly
 */
int sock_init(sock_t * sock);

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
 * Close the socket's connection (if it exists)
 * 
 * Destroy the socket
 * 
 * @param   sock    pointer to socket data structure
 * 
 * @return  0   is connection was closed succesfully
 * @return  -1  on error (errno will be set appropriately)
 */
int sock_destroy(sock_t * sock);

/**
 * Returns the amount of bytes avaiable for reading in the socket's system buffer.
 *
 * In contiki, in order for the process to wait for new data, 
 * the macro PROCES_SOCk_WAIT_DATA must be used beforehand. 
 * 
 * @param   sock    pointer to socket data structure to read from
 * 
 * @return   >=0    number of bytes avaiable
 */
unsigned int sock_poll(sock_t * sock);

/**
 * Read len bytes from the socket into the specified buffer.
 *
 * In contiki, in order for the process to wait for new data, 
 * the macro PROCES_SOCk_WAIT_DATA must be used beforehand. 
 * 
 * @param   sock    pointer to socket data structure to read from
 * @param   buf     buffer to read the data into
 * @param   buf_len number of bytes to read
 * 
 * @return   >=0    number of bytes read
 * @return  -1      on error (errno will be set appropriately)
 */
int sock_read(sock_t * sock, uint8_t * buf, size_t buf_len);

/**
 * Write len bytes from the buffer into the socket. The method 
 * will attempt to write all bytes unless an error ocurrs.
 * 
 * @param   sock    pointer to socket data structure to read from
 * @param   buf     buffer to read the data into
 * @param   buf_len number of bytes to read
 * 
 * @return >=0   number of bytes written
 * @return -1   on error (errno will be set appropriately)
 */
int sock_write(sock_t * sock, uint8_t * buf, size_t buf_len);

/**
 * Open and configure provided socket to act as a server in the specified port.
 * 
 * @param   server  pointer to socket data structure (defined by the implementation)
 * @param   port    port to listen for connections
 * 
 * @return  0   if setting socket to act as a server succeeds
 * @return -1   on error (errno will be set appropriately)
 */
int sock_listen(sock_t * server, uint16_t port);

/**
 * Check avaiable connections on server socket, and create a client for 
 * one of them, if available. Server socket must be  set to listen by 
 * calling sock_listen() previously, otherwise an error will be 
 * returned.
 *
 * In contiki, in order for the process to wait for a client, the m
 * acro PROCES_SOCk_WAIT_CLIENT must be used beforehand. 
 * 
 * @param   server      pointer to server socket data structure (defined by the implementation)
 * @param   client      pointer to client socket data structure (it will be initialized by the function)
 * 
 * @return   1      if a new client was successfully accepted
 * @return   0      if no new clients were available
 * @return  -1      on error (errno will be set appropriately)
 */
int sock_accept(sock_t * server, sock_t * client);

/**
 * Connect the client socket to the specified address and port. 
 * The socket must be initialized with sock_create() previously.
 * 
 * @param   client  pointer to client socket data structure
 * @param   address remote address to connect the socket to
 * @param   port    remote port to connect to
 * 
 * @return   0      if a new connection was successfully established
 * @return  -1      on error (errno will be set appropriately)
 */
int sock_connect(sock_t * client, char * addr, uint16_t port);

#ifdef WITH_CONTIKI
/**
 * Wait for the underlying socket to connect, either 
 * by a new client arriving (in the case of a server sock)
 * or by the remote server becoming available
 *
 * @param sock socket configured and listening
 */ 
PT_THREAD(sock_wait_connection(sock_t * sock));

/**
 * Wait for new data to arrive on the provided client sock
 *
 * @param sock connected client sock
 **/
PT_THREAD(sock_wait_data(sock_t * sock));

/**
 * Process sock macros
 */
#define PROCESS_SOCK_WAIT_CONNECTION(sock) PROCESS_SOCK_WAIT(sock, sock_wait_connection(sock))
#define PROCESS_SOCK_WAIT_DATA(sock) PROCESS_SOCK_WAIT(sock, sock_wait_data(sock))
#endif


#endif /* SOCK_H */
