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
typedef void (*event_connection_cb)(struct event_sock *server, int status);
typedef int (*event_read_cb)(struct event_sock *sock, ssize_t size, uint8_t *bytes);
typedef void (*event_write_cb)(struct event_sock *sock, int status);
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
int event_listen(event_sock_t *sock, uint16_t port, event_connection_cb cb);
int event_read(event_sock_t *sock, event_read_cb cb);
void event_read_stop(event_sock_t *sock);
int event_write(event_sock_t *sock, size_t size, uint8_t *bytes, event_write_cb cb);
int event_close(event_sock_t *sock, event_close_cb cb);
int event_accept(event_sock_t *server, event_sock_t *client);

// Loop operations
void event_loop_init(event_loop_t *loop);
event_sock_t *event_sock_create(event_loop_t *loop);
void event_loop(event_loop_t *loop);

#endif
