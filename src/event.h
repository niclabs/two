#ifndef EVENT_H
#define EVENT_H

#include <sys/types.h>
#include <sys/select.h>
#include <stdint.h>

#include "cbuf.h"

#ifndef CONF_EVENT_MAX_HANDLES
#define EVENT_MAX_HANDLES 8
#else
#define EVENT_MAX_HANDLES (CONF_EVENT_MAX_HANDLES)
#endif

#ifndef CONF_EVENT_MAX_BUF_SIZE
#define EVENT_MAX_BUF_SIZE 128
#else
#define EVENT_MAX_BUF_SIZE (CONF_EVENT_MAX_BUF_SIZE)
#endif

struct event;
struct event_sock;
struct event_loop;

// Callbacks

// Called on a new client connection 
typedef void (*event_connection_cb)(struct event_sock *server, int status);

// Called whenever new data is available, or a read error occurrs 
// it must return the number of bytes read in order to remove them from the input buffer
typedef int (*event_read_cb)(struct event_sock *sock, ssize_t size, uint8_t *bytes);

// Will be called when the output buffer is empty
typedef void (*event_write_cb)(struct event_sock *sock, int status);

// Will be called after all write operations are finished and 
// the socket is closed
typedef void (*event_close_cb)(struct event_sock *sock);

typedef int event_handle_t;

typedef struct event_sock {
    /* "inherited fields" */
    struct event_sock *next;

    /* public fields */
    void *data;

    /* private fields */
    struct event_loop *loop;

    /* type related fields */
    event_handle_t socket;
    enum {
        EVENT_SOCK_CLOSED,
        EVENT_SOCK_OPENED,
        EVENT_SOCK_LISTENING,
        EVENT_SOCK_CONNECTED,
        EVENT_SOCK_CLOSING
    } state;

    /* new connection callback */
    event_connection_cb conn_cb;

    /* read event */
    uint8_t buf_in_data[EVENT_MAX_BUF_SIZE];
    cbuf_t buf_in;
    event_read_cb read_cb;

    /* write event */
    uint8_t buf_out_data[EVENT_MAX_BUF_SIZE];
    cbuf_t buf_out;
    event_write_cb write_cb;

    event_close_cb close_cb;
} event_sock_t;

typedef struct event_loop {
    // list of active sockets
    event_sock_t *polling;

    // static memory
    event_sock_t sockets[EVENT_MAX_HANDLES];
    event_sock_t *unused;

    // list of file descriptors
    fd_set active_fds;
    int nfds;
} event_loop_t;


// Sock operations

// Open the socket for listening on the specified port
// On new client it will call the callback with a 0 status if
// there are free available client slots and -1 if no more
// client slots are available
int event_listen(event_sock_t *sock, uint16_t port, event_connection_cb cb);

// Be notified on read operations on the socket
// event_read_stop must be called before assigning a new read callback
int event_read(event_sock_t *sock, event_read_cb cb);

// Stop receiving read notifications
void event_read_stop(event_sock_t *sock);

// Write to the output buffer, will notify the callback when all bytes are 
// written
int event_write(event_sock_t *sock, size_t size, uint8_t *bytes, event_write_cb cb);

// Close the socket
// will notify the callback after all write operations are finished
int event_close(event_sock_t *sock, event_close_cb cb);

// Accept a new client
int event_accept(event_sock_t *server, event_sock_t *client);

// Loop operations

// Initialize a new event_loop
void event_loop_init(event_loop_t *loop);

// Obtain a new free socket from the event loop
// it will fail if there are no more sockets available
event_sock_t *event_sock_create(event_loop_t *loop);

// Start the loop
void event_loop(event_loop_t *loop);

#endif
