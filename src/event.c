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

// Private methods
struct qnode {
    struct qnode *next;
};

typedef int (*event_compare_cb)(void *first, void *second);

void *event_queue_find(struct qnode *queue, void *elem, event_compare_cb compare)
{
    struct qnode *head = queue;

    while (head != NULL) {
        if (compare(head, elem) > 0) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

void *event_queue_delete(struct qnode *queue, struct qnode *elem)
{
    struct qnode *head = queue;
    struct qnode *prev = head;

    while (head != NULL) {
        if (head == elem) {
            prev->next = head->next;
            head->next = NULL;

            return head;
        }
        prev = head;
        head = head->next;
    }

    return NULL;
}

int compare_handle(void *s, void *h)
{
    event_sock_t *sock = (event_sock_t *)s;
    event_handle_t *handle = (event_handle_t *)h;

    return sock->socket == *handle;
}

// find socket file descriptor in socket queue
event_sock_t *event_find_handle(event_sock_t *queue, event_handle_t handle)
{
    return event_queue_find((struct qnode *)queue, &handle, &compare_handle);
}

void event_do_read(event_sock_t *sock)
{
    // this is a server socket
    if (sock->state == EVENT_SOCK_LISTENING && sock->conn_cb != NULL) {
        // call connection_cb with status ok if there are available clients
        sock->conn_cb(sock, sock->loop->unused == NULL ? -1 : 0);
    }
    else if (sock->read_cb != NULL) {
        int readlen = EVENT_MAX_BUF_SIZE - cbuf_len(&sock->buf_in);
        uint8_t buf[readlen];

        // perform read in non blocking manner
        int count = recv(sock->socket, buf, readlen, MSG_DONTWAIT);
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

void event_do_write(event_sock_t *sock)
{
    if (sock->write_cb != NULL) {
        int len = cbuf_len(&sock->buf_out);
        uint8_t buf[len];

        cbuf_peek(&sock->buf_out, buf, len);
        int written = send(sock->socket, buf, len, MSG_DONTWAIT);

        if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            sock->write_cb(sock, -1);
            return;
        }

        // remove written
        cbuf_pop(&sock->buf_out, NULL, written);

        // notify of write when buffer is empty
        if (cbuf_len(&sock->buf_out) <= 0) {
            sock->write_cb(sock, 0);

            // remove write callback
            sock->write_cb = NULL;
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

        if (!read && !write) {
            continue;
        }

        // find the handle on the polling list
        event_sock_t *sock = event_find_handle(loop->polling, i);

        // if this happens it means there is a problem with the implementation
        assert(sock != NULL);

        // if a read event is detected perform read operations
        if (read) {
            event_do_read(sock);
        }

        // if a write event is detected perform write operations
        if (write) {
            event_do_write(sock);
        }
    }
}

void event_loop_close(event_loop_t *loop)
{
    event_sock_t *curr = loop->polling;
    event_sock_t *prev = NULL;

    int max_fds = -1;

    while (curr != NULL) {
        // if the event is closing and we are not waiting to read
        if (curr->state == EVENT_SOCK_CLOSING && cbuf_len(&curr->buf_out) <= 0) {
            // prevent sock to be used in polling
            FD_CLR(curr->socket, &loop->active_fds);

            // close the socket and update its status
            close(curr->socket);
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

            // move the head forward
            curr = next;
            continue;
        }

        if (curr->socket > max_fds) {
            max_fds = curr->socket;
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
    sock->socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock->socket < 0) {
        return -1;
    }

    // allow address reuse to prevent "address already in use" errors
    int option = 1;
    setsockopt(sock->socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* Struct sockaddr_in6 needed for binding. Family defined for ipv6. */
    struct sockaddr_in6 sin6;
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(port);
    sin6.sin6_addr = in6addr_any;

    if (bind(sock->socket, (struct sockaddr *)&sin6, sizeof(sin6)) < 0) {
        close(sock->socket);
        return -1;
    }

    if (listen(sock->socket, EVENT_MAX_HANDLES - 1) < 0) {
        close(sock->socket);
        return -1;
    }

    // update max fds value
    loop->nfds = sock->socket + 1;

    // add socket fd to read watchlist
    FD_SET(sock->socket, &loop->active_fds);

    // add poll callback to sock
    sock->conn_cb = cb;

    // update sock state
    sock->state = EVENT_SOCK_LISTENING;

    // add socket to polling queue
    sock->next = loop->polling;
    loop->polling = sock;

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

    // check that we are not already reading
    assert(sock->read_cb == NULL);

    // add read callback to event
    sock->read_cb = cb;

    // if there are bytes available in the buffer
    // call the read callback immediately
    int len = cbuf_len(&sock->buf_in);
    if (len > 0) {
        uint8_t buf[len];

        // get the remaining bytes in the buffer
        cbuf_peek(&sock->buf_in, buf, len);

        // notify the calling event
        int readlen = sock->read_cb(sock, len, buf);

        // remove the read bytes
        cbuf_pop(&sock->buf_in, NULL, readlen);
    }

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

    // check that we are not already writing
    assert(sock->write_cb == NULL);

    // set write cb
    sock->write_cb = cb;

    // write bytes to output buffer
    return cbuf_push(&sock->buf_out, bytes, size);
}

int event_accept(event_sock_t *server, event_sock_t *client)
{
    assert(server != NULL && client != NULL);
    assert(server->loop != NULL && client->loop != NULL);

    // check correct socket state
    assert(server->state == EVENT_SOCK_LISTENING);
    assert(client->state == EVENT_SOCK_CLOSED);

    int clifd = accept(server->socket, NULL, NULL);
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
    client->socket = clifd;
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
    memset(&loop->sockets, 0, EVENT_MAX_HANDLES * sizeof(event_sock_t));

    // create unused socket list
    for (int i = 0; i < EVENT_MAX_HANDLES - 1; i++) {
        loop->sockets[i].next = &loop->sockets[i + 1];
    }

    // list of unused sockets
    loop->unused = &loop->sockets[0];
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
    cbuf_init(&sock->buf_out, sock->buf_out_data, EVENT_MAX_BUF_SIZE);

    // assign sock variables
    sock->loop = loop;
    sock->state = EVENT_SOCK_CLOSED;

    return sock;
}

void event_loop(event_loop_t *loop)
{
    while (event_loop_is_alive(loop)) {
        // perform timer events

        // perform I/O events
        event_loop_poll(loop);

        // perform close events
        event_loop_close(loop);
    }
}
