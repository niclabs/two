#ifndef NET_H
#define NET_H

#include <stdio.h>
#include "config.h"

/*
* Amount of clients.
*/
#ifndef NET_MAX_CLIENTS
#define NET_MAX_CLIENTS ((unsigned int) 50)
#endif

/*
* Enum for possible error cases within a connection loop.
*/
typedef enum
{
    NET_OK = 0,
    NET_SOCKET_ERROR = -1,
    NET_READ_ERROR = -2,
    NET_WRITE_ERROR = -3,
} net_return_code_t;

/*
* Main loop of a server.
*
* port:                 port to listen from
* default_callback:     Callback a client executes on connection
* stop_flag:            Stops the server loop when set to 1 from outside
* data_buffer_size      Size in bytes for the client's in and out buffers
* client_state_size     Size in bytes to reserve for the client's state.
*/
net_return_code_t net_server_loop(unsigned int port, callback_t default_callback, int* stop_flag, size_t data_buffer_size, size_t client_state_size);

#endif
