#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#ifndef CONTIKI
#include <sys/select.h>
#include <sys/time.h>
#else
#include "net/ipv6/tcpip.h"
#endif

#include "two-conf.h"

#include "cbuf.h"
#include "ll.h"

#ifndef EVENT_MAX_DESCRIPTORS
#define EVENT_MAX_DESCRIPTORS 4
#endif

// event max sockets should be at least 2 (1 server and 1 client)
#ifndef EVENT_MAX_SOCKETS
#define EVENT_MAX_SOCKETS (EVENT_MAX_DESCRIPTORS)
#endif

// event max should be at least 3
// 1 listen event for a server socket
// 1 read and 1 write event for a client socket
#ifndef EVENT_MAX_EVENTS
#define EVENT_MAX_EVENTS (EVENT_MAX_SOCKETS * 3)
#endif

// Defines the total number of write operation handlers
#ifndef EVENT_WRITE_QUEUE_SIZE
#define EVENT_WRITE_QUEUE_SIZE (4 * EVENT_MAX_SOCKETS)
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

// Called whenever a write operation is performed. If an error occurs
// Status will be <0
// Remaining contains the number of remaining bytes in the write buffer
typedef void (*event_write_cb)(struct event_sock *sock, int status);

// Will be called after all write operations are finished and
// the socket is closed
typedef void (*event_close_cb)(struct event_sock *sock);

// Will be called after timer events are due
// it must return 1 to stop the callback, 0 to reset
// the timer
typedef int (*event_timer_cb)(struct event_sock *sock);

#ifdef CONTIKI
typedef uint16_t event_descriptor_t;
#else
typedef int event_descriptor_t;
#endif

// 1-byte alignment
#pragma pack(push, 1)

typedef enum {
    EVENT_CONNECTION_TYPE   = (uint8_t) 0x0,
    EVENT_WRITE_TYPE        = (uint8_t) 0x1,
    EVENT_READ_TYPE         = (uint8_t) 0x2,
    EVENT_TIMER_TYPE        = (uint8_t) 0x3,
} event_type_t;

typedef struct event_connection {
    // type variables
    event_connection_cb cb;
} event_connection_t;

typedef struct event_read {
    // type variables
    cbuf_t buf;
    event_read_cb cb;
} event_read_t;

typedef struct event_write_op {
    struct event_write_op *next;
    unsigned int bytes;
    event_write_cb cb;
} event_write_op_t;

typedef struct event_write {
    // type variables
    cbuf_t buf;
    event_write_op_t *queue;
#ifdef CONTIKI
    // > 0 if there is data waiting
    // to be acked
    unsigned int sending;
#endif
} event_write_t;

typedef struct event_timer {
    // type variables
#ifdef CONTIKI
    struct ctimer ctimer;
#else
    int millis;
    struct timeval start;
#endif
    event_timer_cb cb;
} event_timer_t;

typedef struct event {
    struct event *next;
    struct event_sock *sock;
    event_type_t type;
    union {
        event_connection_t connection;
        event_read_t read;
        event_write_t write;
        event_timer_t timer;
    } data;
} event_t;

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

    // event list
    event_t *events;

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
    LL(event_sock_t, sockets, EVENT_MAX_SOCKETS);
    LL(event_t, events, EVENT_MAX_EVENTS);
    LL(event_write_op_t, writes, EVENT_WRITE_QUEUE_SIZE);

    // loop state
    int running;

#ifndef CONTIKI
    // list of file descriptors
    fd_set active_fds;
    int nfds;
#endif
} event_loop_t;

// end byte alignment
#pragma pack(pop)

// Sock operations

// Open the socket for listening on the specified port
// On new client it will call the callback with a 0 status if
// there are free available client slots and -1 if no more
// client slots are available
int event_listen(event_sock_t *sock, uint16_t port, event_connection_cb cb);

// Start reading events in the socket. This configures the given buffer for reading,
// this will configure the buffer as a circular buffer until event_read_stop is called,
// where the handler will be released and writing on the buffer will stop. Memory
// allocation/freeing is responsibility of the user of the library
//
// event_read_cb will be called on new data
void event_read_start(event_sock_t *sock, uint8_t *buf, unsigned int bufsize, event_read_cb cb);

// Update the notification callback for read operations in the given socket
// event_read_start MUST be called first
int event_read(event_sock_t *sock, event_read_cb cb);

// Stop receiving read notifications
// this releases the read handler and stops writing
// in the buffer given at event_read_start.
// If any data are left in the buffer, this will
// leave them untouched at the beginning of the memory pointed
// by the buffer and return the number of bytes available
int event_read_stop(event_sock_t *sock);

// Enable the socket for writing, and provide memory for buffering writing events
// event_write will fail (assert error) if event_write is called before event_write_alloc
// the allocated memory will be freed once the socket is successfull closed
// the callback will be called on succesful write or on socket error
void event_write_enable(event_sock_t *sock, uint8_t *buf, unsigned int bufsize);

// Write to the output buffer, will notify the callback when all bytes are
// written
int event_write(event_sock_t *sock, unsigned int size, uint8_t *bytes, event_write_cb cb);

// Notify the callback on elapsed time
event_t *event_timer(event_sock_t *sock, unsigned int millis, event_timer_cb cb);

// Reset the given timer (will fail if called event other than a timer)
void event_timer_reset(event_t *timer);

// Stop the given timer (will fail if called event other than a timer)
void event_timer_stop(event_t *timer);

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
int event_sock_unused(event_loop_t *loop);

// Start the loop
void event_loop(event_loop_t *loop);

#endif
