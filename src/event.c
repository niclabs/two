#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "event.h"

#define LOG_MODULE EVENT
#define LOG_LEVEL_EVENT LOG_LEVEL_DEBUG
#include "logging.h"

#define LIST_ADD(list, elem) \
    {                        \
        elem->next = list;   \
        list = elem;         \
    }
#define LIST_NEXT(elem) elem = elem->next;
#define LIST_FIND(type, queue, condition)   \
    ({                                      \
        type *elem = queue;                 \
        type *res = NULL;                   \
        while (elem != NULL) {              \
            if (condition) {                \
                res = elem;                 \
                break;                      \
            }                               \
            LIST_NEXT(elem);                \
        }                                   \
        res;                                \
    });

#define LIST_DELETE(type, queue, elem)          \
    ({                                          \
        type *res = NULL;                       \
        type *curr = queue, *prev = NULL;       \
        while (curr != NULL) {                  \
            if (curr == elem) {                 \
                if (prev == NULL) {             \
                    queue = curr->next;         \
                }                               \
                else {                          \
                    prev->next = curr->next;    \
                }                               \
                res = curr;                     \
                break;                          \
            }                                   \
            LIST_NEXT(elem);                    \
        }                                       \
        res;                                    \
    });

// Private methods
struct qnode {
    struct qnode *next;
};

typedef int (*event_compare_cb)(void *first, void *second);

// find socket file descriptor in socket queue
event_sock_t *event_socket_find(event_sock_t *queue, event_descriptor_t descriptor)
{
    return LIST_FIND(event_sock_t, queue, elem->descriptor == descriptor);
}

// find socket file descriptor in socket queue
event_handler_t *event_handler_find(event_handler_t *queue, event_type_t type)
{
    return LIST_FIND(event_handler_t, queue, elem->type == type);
}

// find free handler in loop memory
event_handler_t *event_handler_find_free(event_loop_t *loop, event_sock_t *sock)
{
    if (loop->unused_handlers == NULL) {
        return NULL;
    }

    // get event handler from loop memory
    event_handler_t *handler = loop->unused_handlers;
    loop->unused_handlers = handler->next;

    // reset handler memory
    memset(handler, 0, sizeof(event_handler_t));

    handler->next = sock->handlers;
    sock->handlers = handler;

    return handler;
}


void event_do_read(event_sock_t *sock)
{
    // this is a server socket
    if (sock->state == EVENT_SOCK_LISTENING) {
        // call connection_cb with status ok if there are available clients
        event_handler_t *handler = event_handler_find(sock->handlers, EVENT_CONNECTION_TYPE);
        if (handler != NULL) {
            // if this happens there is an error with the implementation
            assert(handler->event.connection.cb != NULL);
            handler->event.connection.cb(sock, sock->loop->unused == NULL ? -1 : 0);
        }
    }
    else if (sock->read_cb != NULL) {
        int readlen = EVENT_MAX_BUF_SIZE - cbuf_len(&sock->buf_in);
        uint8_t buf[readlen];

        // perform read in non blocking manner
        int count = recv(sock->descriptor, buf, readlen, MSG_DONTWAIT);
        if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            sock->read_cb(sock, count, NULL);
            return;
        }

        // push data into buffer
        cbuf_push(&sock->buf_in, buf, count);

        int buflen = cbuf_len(&sock->buf_in);
        uint8_t read_buf[buflen];
        cbuf_peek(&sock->buf_in, read_buf, buflen);

        // notify about the new data
        readlen = sock->read_cb(sock, buflen, read_buf);

        // remove the read bytes from the buffer
        cbuf_pop(&sock->buf_in, NULL, readlen);
    }
}

void event_do_write(event_sock_t *sock, event_handler_t *handler)
{
    if (handler != NULL) {
        // if this happens there is a problem with the implementation
        assert(handler->event.write.cb != NULL);

        // attempt to write remaining bytes
        int len = cbuf_len(&handler->event.write.buf);
        uint8_t buf[len];

        cbuf_peek(&handler->event.write.buf, buf, len);
        int written = send(sock->descriptor, buf, len, MSG_DONTWAIT);

        if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            handler->event.write.cb(sock, -1);
            return;
        }

        // remove written
        cbuf_pop(&handler->event.write.buf, NULL, written);

        // notify of write when buffer is empty
        if (cbuf_len(&handler->event.write.buf) <= 0) {
            // remove write handler from list
            LIST_DELETE(event_handler_t, sock->handlers, handler);

            // notify of write
            handler->event.write.cb(sock, 0);

            // move handler to loop unused list
            LIST_ADD(sock->loop->unused_handlers, handler);
        }
    }
}

void event_loop_poll(event_loop_t *loop)
{
    assert(loop->nfds >= 0);

    fd_set read_fds = loop->active_fds;
    fd_set write_fds = loop->active_fds;
    struct timeval tv = { 0, 0 };     // how much time should we block?

    // poll list of file descriptors with select
    if (select(loop->nfds, &read_fds, &write_fds, NULL, &tv) < 0) {
        // if an error different from interrupt is caught
        // here, there is an issue with the implementation
        assert(errno == EINTR);
        return;
    }

    for (int i = 0; i < loop->nfds; i++) {
        int read = 0,  write = 0;

        // check read operations
        if (FD_ISSET(i, &read_fds)) {
            read = 1;
        }

        // check write operations
        if (FD_ISSET(i, &write_fds)) {
            write = 1;
        }

        event_sock_t *sock;
        event_handler_t *wh;
        if (read || write || FD_ISSET(i, &loop->active_fds)) {
            // find the handle on the polling list
            sock = event_socket_find(loop->polling, i);

            // if this happens it means there is a problem with the implementation
            assert(sock != NULL);

            if (cbuf_len(&sock->buf_in) > 0) {
                read = 1;
            }

            wh = event_handler_find(sock->handlers, EVENT_WRITE_TYPE);
            if (wh != NULL && cbuf_len(&wh->event.write.buf) > 0) {
                write = 1;
            }
        }

        if (!read && !write) {
            continue;
        }

        // if a read event is detected perform read operations
        if (read) {
            event_do_read(sock);
        }

        // if a write event is detected perform write operations
        if (write) {
            event_do_write(sock, wh);
        }
    }
}

void event_loop_close(event_loop_t *loop)
{
    event_sock_t *curr = loop->polling;
    event_sock_t *prev = NULL;

    int max_fds = -1;

    while (curr != NULL) {
        // if the event is closing and we are not waiting to write
        event_handler_t *wh = event_handler_find(curr->handlers, EVENT_WRITE_TYPE);
        if (curr->state == EVENT_SOCK_CLOSING && (wh == NULL || cbuf_len(&wh->event.write.buf) <= 0)) {
            // prevent sock to be used in polling
            FD_CLR(curr->descriptor, &loop->active_fds);

            // close the socket and update its status
            close(curr->descriptor);
            curr->state = EVENT_SOCK_CLOSED;

            // call the close callback
            curr->close_cb(curr);

            // Move curr socket to unused list
            event_sock_t *next = curr->next;
            curr->next = loop->unused;
            loop->unused = curr;

            // remove the socket from the polling list
            // and move the socket back to the unused list
            if (prev == NULL) {
                loop->polling = next;
            }
            else {
                prev->next = next;
            }

            // move socket handlers back to the unused handler list
            for (event_handler_t *h = curr->handlers; h != NULL; h = h->next) {
                h->next = loop->unused_handlers;
                loop->unused_handlers = h;
            }

            // move the head forward
            curr = next;
            continue;
        }

        if (curr->descriptor > max_fds) {
            max_fds = curr->descriptor;
        }

        prev = curr;
        curr = curr->next;
    }

    // update the max file descriptor
    loop->nfds = max_fds + 1;
}

int event_loop_is_alive(event_loop_t *loop)
{
    return loop->polling != NULL;
}


// Public methods
int event_listen(event_sock_t *sock, uint16_t port, event_connection_cb cb)
{
    assert(sock != NULL);
    assert(cb != NULL);
    assert(sock->loop != NULL);
    assert(sock->state == EVENT_SOCK_CLOSED);

    event_loop_t *loop = sock->loop;
    sock->descriptor = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock->descriptor < 0) {
        return -1;
    }

    // allow address reuse to prevent "address already in use" errors
    int option = 1;
    setsockopt(sock->descriptor, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* Struct sockaddr_in6 needed for binding. Family defined for ipv6. */
    struct sockaddr_in6 sin6;
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(port);
    sin6.sin6_addr = in6addr_any;

    if (bind(sock->descriptor, (struct sockaddr *)&sin6, sizeof(sin6)) < 0) {
        close(sock->descriptor);
        return -1;
    }

    if (listen(sock->descriptor, EVENT_MAX_SOCKETS - 1) < 0) {
        close(sock->descriptor);
        return -1;
    }

    // update max fds value
    loop->nfds = sock->descriptor + 1;

    // add socket fd to read watchlist
    FD_SET(sock->descriptor, &loop->active_fds);

    // update sock state
    sock->state = EVENT_SOCK_LISTENING;

    // add socket to polling queue
    sock->next = loop->polling;
    loop->polling = sock;

    // add event handler to socket
    event_handler_t *handler = event_handler_find_free(loop, sock);
    assert(handler != NULL);

    // set handler event
    handler->type = EVENT_CONNECTION_TYPE;
    memset(&handler->event.connection, 0, sizeof(event_connection_t));
    handler->event.connection.cb = cb;

    return 0;
}

int event_read(event_sock_t *sock, event_read_cb cb)
{
    // check socket status
    assert(sock != NULL);
    assert(sock->loop != NULL);
    assert(cb != NULL);

    // read can only be performed on connected sockets
    assert(sock->state == EVENT_SOCK_CONNECTED);

    // add read callback to event
    sock->read_cb = cb;

    return 0;
}

void event_read_stop(event_sock_t *sock)
{
    assert(sock != NULL);
    assert(sock->loop != NULL);

    // do cleanup
    sock->read_cb = NULL;
}

int event_write(event_sock_t *sock, size_t size, uint8_t *bytes, event_write_cb cb)
{
    // check socket status
    assert(sock != NULL);
    assert(sock->loop != NULL);

    // write can only be performed on a connected socket
    assert(sock->state == EVENT_SOCK_CONNECTED);

    // see if there is already a write handler set
    event_handler_t *handler = event_handler_find(sock->handlers, EVENT_WRITE_TYPE);
    assert(handler == NULL);

    // find free handler
    event_loop_t *loop = sock->loop;
    handler = event_handler_find_free(loop, sock);
    assert(handler != NULL);

    // set handler event
    handler->type = EVENT_WRITE_TYPE;
    memset(&handler->event.write, 0, sizeof(event_write_t));

    // initialize write buffer
    cbuf_init(&handler->event.write.buf, handler->event.write.buf_data, EVENT_MAX_BUF_SIZE);

    // set write callback
    handler->event.write.cb = cb;

    // write bytes to output buffer
    return cbuf_push(&handler->event.write.buf, bytes, size);
}

int event_accept(event_sock_t *server, event_sock_t *client)
{
    assert(server != NULL && client != NULL);
    assert(server->loop != NULL && client->loop != NULL);

    // check correct socket state
    assert(server->state == EVENT_SOCK_LISTENING);
    assert(client->state == EVENT_SOCK_CLOSED);

    int clifd = accept(server->descriptor, NULL, NULL);
    if (clifd < 0) {
        ERROR("Failed to accept new client");
        return -1;
    }

    event_loop_t *loop = client->loop;

    // update max fds value
    loop->nfds = clifd + 1;

    // add socket fd to read watchlist
    FD_SET(clifd, &loop->active_fds);

    // set client variables
    client->descriptor = clifd;
    client->state = EVENT_SOCK_CONNECTED;

    // add socket to polling list
    client->next = loop->polling;
    loop->polling = client;

    return 0;
}

int event_close(event_sock_t *sock, event_close_cb cb)
{
    // check socket status
    assert(sock != NULL);
    assert(sock->loop != NULL);
    assert(sock->state == EVENT_SOCK_CONNECTED || \
           sock->state == EVENT_SOCK_LISTENING);
    assert(cb != NULL);

    // set sock state and callback
    sock->state = EVENT_SOCK_CLOSING;
    sock->close_cb = cb;

    return 0;
}

void event_loop_init(event_loop_t *loop)
{
    assert(loop != NULL);
    // fail if loop pointer is not initialized

    // reset loop memory
    memset(loop, 0, sizeof(event_loop_t));

    // reset socket memory
    FD_ZERO(&loop->active_fds);
    memset(loop->sockets, 0, EVENT_MAX_SOCKETS * sizeof(event_sock_t));

    // create unused socket list
    for (int i = 0; i < EVENT_MAX_SOCKETS - 1; i++) {
        loop->sockets[i].next = &loop->sockets[i + 1];
    }
    // list of unused sockets
    loop->unused = &loop->sockets[0];

    // reset handler memory and create unused handler list
    memset(loop->handlers, 0, EVENT_MAX_HANDLERS * sizeof(event_handler_t));
    for (int i = 0; i  < EVENT_MAX_HANDLERS - 1; i++) {
        loop->handlers[i].next = &loop->handlers[i + 1];
    }
    loop->unused_handlers = &loop->handlers[0];
}

event_sock_t *event_sock_create(event_loop_t *loop)
{
    assert(loop != NULL);

    // check that the loop is initialized
    // and there are available clients
    assert(loop->unused != NULL);

    // get the first unused sock
    // and remove sock from unsed list
    event_sock_t *sock = loop->unused;
    loop->unused = sock->next;

    // reset sock memory
    memset(sock, 0, sizeof(event_sock_t));

    // initialize buffers
    cbuf_init(&sock->buf_in, sock->buf_in_data, EVENT_MAX_BUF_SIZE);

    // assign sock variables
    sock->loop = loop;
    sock->state = EVENT_SOCK_CLOSED;

    return sock;
}

void event_loop(event_loop_t *loop)
{
    assert(loop != NULL);
    assert(loop->running == 0);

    loop->running = 1;
    while (event_loop_is_alive(loop)) {
        // perform timer events

        // perform I/O events
        event_loop_poll(loop);

        // perform close events
        event_loop_close(loop);
    }
    loop->running = 0;
}
