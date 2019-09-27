#include <stdint.h>

typedef struct sock_sock_struct sock_t;

/*
Creates a non blocking socket.

Returns 0   if Ok
        -1  if Error
*/
int sock_create(sock_t* socket);

/*
Sets a socket to listen mode.

Returns 0   if Ok
        -1  if Error
*/
int sock_listen(sock_t* server_socket, uint16_t port);

/*
Checks if there's new data to be read from the socket.

Returns >=0 the amount of data available
*/
int sock_poll(sock_t* socket);

/*
Reads from a socket without blocking.

Returns -1  if Error
        >=0 the amount of bytes read
*/
int sock_read(sock_t* socket, char * buffer, unsigned int buffer_length);

/*
Writes into a socket without blocking.

Returns -1  if Error
        >=0 the amount of bytes written
*/
int sock_write(sock_t* socket, char * buffer, unsigned int buffer_length);

/*
Accepts a client into a socket without blocking.

Returns 1   if a client was accepted
        0   if no client was accepted
        -1  if Error
*/
int sock_accept(sock_t* server_socket, sock_t* client_socket);