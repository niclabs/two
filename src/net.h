#include "cbuf.h"
#include "config.h"

#ifndef NET_H
#define NET_H

/*
* Amount of clients.
*/
#ifndef NET_MAX_CLIENTS
#define NET_MAX_CLIENTS ((unsigned int) 2)
#endif

/*
* Signature of a callback
*
* buf_in:       has size NET_CLIENT_BUFFER_SIZE, net writes into it
* buf_out:      has size NET_CLIENT_BUFFER_SIZE, net reads from it
* state:        has size NET_CLIENT_STATE_SIZE, net doesn't touch it. inits to 0
*/
typedef void* (*net_Callback) (cbuf_t* buf_in, cbuf_t* buf_out, void* state);

/*
* Enum for possible error cases within a connection looṕ.
*/
typedef enum
{
    Ok = 0,
    SocketError = -1,
    ReadError = -2,
    WriteError = -3,
} NetReturnCode;

/*
* Main loop of a server.
*
* port:                 port to listen from
* default_callback:     Callback a client executes on connection
* stop_flag:            Stops the server loop when set to 1 from outside
* data_buffer_size      Size in bytes for the client's in and out buffers
* client_state_size     Size in bytes to reserve for the client's state.
*/
NetReturnCode net_server_loop(unsigned int port, callback_t default_callback, int* stop_flag, size_t data_buffer_size, size_t client_state_size);

#endif
