#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#ifndef CONTIKI
#include <sys/select.h>
#else
#include "net/ipv6/tcpip.h"
#endif

#include "cbuf.h"

#ifndef CONF_EVENT_MAX_DESCRIPTORS
#define EVENT_MAX_DESCRIPTORS 3
#else
#define EVENT_MAX_DESCRIPTORS (CONF_EVENT_MAX_DESCRIPTORS)
#endif

// event max sockets should be at least 2 (1 server and 1 client)
#ifndef CONF_EVENT_MAX_SOCKETS
#define EVENT_MAX_SOCKETS (EVENT_MAX_DESCRIPTORS)
#else
#define EVENT_MAX_SOCKETS (CONF_EVENT_MAX_SOCKETS)
#endif

// event max handlers should be at least 3
// 1 listen handler for a server socket
// 1 read and 1 write handler for a client socket
#ifndef CONF_EVENT_MAX_HANDLERS
#define EVENT_MAX_HANDLERS (EVENT_MAX_SOCKETS * 2)
#else
#define EVENT_MAX_HANDLERS (CONF_EVENT_MAX_HANDLERS)
#endif

#ifndef CONF_EVENT_MAX_BUF_READ_SIZE
#define EVENT_MAX_BUF_READ_SIZE 4096
#else
#define EVENT_MAX_BUF_READ_SIZE (CONF_EVENT_MAX_BUF_READ_SIZE)
#endif

#ifndef CONF_EVENT_MAX_BUF_WRITE_SIZE
#define EVENT_MAX_BUF_WRITE_SIZE 64
#else
#define EVENT_MAX_BUF_WRITE_SIZE (CONF_EVENT_MAX_BUF_WRITE_SIZE)
#endif

struct event;
struct event_sock;
struct event_loop;

// Callbacks

// Called on a new client connection
typedef void (*event_connection_cb)(struct event_sock *server, int status);

// Called whenever new data is available, or a read error occurrs
// it must return the number of bytes read in order to remove them from the input buffer
typedef int (*event_read_cb)(struct event_sock *sock, int size, uint8_t *bytes);

// Will be called when the output buffer is empty
typedef void (*event_write_cb)(struct event_sock *sock, int status);

// Will be called after all write operations are finished and
// the socket is closed
typedef void (*event_close_cb)(struct event_sock *sock);

#ifdef CONTIKI
typedef uint16_t event_descriptor_t;
#else
typedef int event_descriptor_t;
#endif

typedef enum {
    EVENT_CONNECTION_TYPE,
    EVENT_WRITE_TYPE,
    EVENT_READ_TYPE
} event_type_t;

typedef struct event_connection {
    // type variables
    event_connection_cb cb;
} event_connection_t;

typedef struct event_read {
    // type variables
    uint8_t buf_data[EVENT_MAX_BUF_READ_SIZE];
    cbuf_t buf;
    event_read_cb cb;
} event_read_t;

typedef struct event_write {
    // type variables
    uint8_t buf_data[EVENT_MAX_BUF_WRITE_SIZE];
    cbuf_t buf;
    event_write_cb cb;
#ifdef CONTIKI
    int sending;
#endif
} event_write_t;

typedef struct event_handler {
    struct event_handler *next;
    event_type_t type;
    union {
        event_connection_t connection;
        event_read_t read;
        event_write_t write;
    } event;
} event_handler_t;

typedef struct event_sock {
    /* "inherited fields" */
    struct event_sock *next;

    /* public fields */
    void *data;

    /* private fields */
    struct event_loop *loop;

    /* type related fields */
    event_descriptor_t descriptor;
    enum {
        EVENT_SOCK_CLOSED,
        EVENT_SOCK_OPENED,
        EVENT_SOCK_LISTENING,
        EVENT_SOCK_CONNECTED,
        EVENT_SOCK_CLOSING
    } state;

    // event handler list
    event_handler_t *handlers;

    // close operation
    event_close_cb close_cb;

#ifdef CONTIKI
    struct uip_conn *uip_conn;
#endif
} event_sock_t;

typedef struct event_loop {
    // list of active sockets
    event_sock_t *polling;

    // static memory
    event_sock_t sockets[EVENT_MAX_SOCKETS];
    event_sock_t *unused;

    event_handler_t handlers[EVENT_MAX_HANDLERS];
    event_handler_t *unused_handlers;

    // loop state
    int running;

#ifndef CONTIKI
    // list of file descriptors
    fd_set active_fds;
    int nfds;
#endif
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
int event_write(event_sock_t *sock, unsigned int size, uint8_t *bytes, event_write_cb cb);

// Remove the read callback and set a write callback
int event_read_stop_and_write(event_sock_t *sock, unsigned int size, uint8_t *bytes, event_write_cb cb);

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

// Return number of available free sockets
int event_sock_unused(event_loop_t * loop);

// Start the loop
void event_loop(event_loop_t *loop);

#endif
