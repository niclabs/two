#include <errno.h>

#ifdef CONTIKI
#include "contiki-net.h"
#include "lib/assert.h"
#else
#include <assert.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "event.h"
#include "config.h"

#define LOG_MODULE LOG_MODULE_EVENT

#include "logging.h"
#include "list_macros.h"

#ifdef CONTIKI
// Main contiki process
PROCESS(event_loop_process, "Event loop process");
#endif

/////////////////////////////////////////////////////
// Private methods
/////////////////////////////////////////////////////

// find socket file descriptor in socket queue
event_sock_t *event_socket_find(event_sock_t *queue, event_descriptor_t descriptor)
{
    return LIST_FIND(queue, LIST_ELEM(event_sock_t)->descriptor == descriptor);
}

// find socket file descriptor in socket queue
event_handler_t *event_handler_find(event_handler_t *queue, event_type_t type)
{
    return LIST_FIND(queue, LIST_ELEM(event_handler_t)->type == type);
}

// find free handler in loop memory
event_handler_t *event_handler_find_free(event_loop_t *loop, event_sock_t *sock)
{
    // get event handler from loop memory
    event_handler_t *handler = LIST_POP(loop->unused_handlers);

    if (handler != NULL) {
        // reset handler memory
        memset(handler, 0, sizeof(event_handler_t));

        // link handler to sock memory
        LIST_APPEND(handler, sock->handlers);
    }

    return handler;
}

void event_sock_connect(event_sock_t *sock, event_handler_t *handler)
{
    if (handler != NULL) {
        // if this happens there is an error with the implementation
        assert(handler->event.connection.cb != NULL);
        if (sock->loop->unused != NULL) {
            handler->event.connection.cb(sock, 0);
        }
    }
}

void event_sock_read(event_sock_t *sock, event_handler_t *handler)
{
    assert(sock != NULL);
    if (handler == NULL) {
        return;
    }

    // check available buffer data
    int readlen = EVENT_MAX_BUF_READ_SIZE - cbuf_len(&handler->event.read.buf);
    uint8_t buf[readlen];

#ifdef CONTIKI
    int count = MIN(readlen, uip_datalen());
    memcpy(buf, uip_appdata, count);
#else
    // perform read in non blocking manner
    int count = recv(sock->descriptor, buf, readlen, MSG_DONTWAIT);
    if ((count < 0 && errno != EAGAIN && errno != EWOULDBLOCK) || count == 0) {
        cbuf_end(&handler->event.read.buf);
    }
#endif
    // push data into buffer
    cbuf_push(&handler->event.read.buf, buf, count);
    DEBUG("READ %d bytes", count);
}

void event_sock_read_notify(event_sock_t *sock, event_handler_t *handler)
{
    assert(sock != NULL);
    if (handler == NULL) {
        return;
    }

    int buflen = cbuf_len(&handler->event.read.buf);

    // if a read is not paused notify about the new data
    if (handler->event.read.cb != NULL && (buflen > 0 || cbuf_has_ended(&handler->event.read.buf))) {
        uint8_t read_buf[buflen];
        cbuf_peek(&handler->event.read.buf, read_buf, buflen);

        DEBUG("Calling read callback with %d bytes", buflen);
        int readlen = handler->event.read.cb(sock, buflen, read_buf);
        DEBUG("Read callback read %d bytes", readlen);

        // remove the read bytes from the buffer
        cbuf_pop(&handler->event.read.buf, NULL, readlen);
    }
}

void event_sock_write_notify(event_sock_t *sock, event_handler_t *handler, int status)
{
    assert(sock != NULL);
    if (handler == NULL) {
        return;
    }

    // remove write handler from list
    LIST_DELETE(handler, sock->handlers);

    // notify of write if a handler is available
    if (handler->event.write.cb != NULL) {
        handler->event.write.cb(sock, status);
    }

    // move handler to loop unused list
    LIST_PUSH(handler, sock->loop->unused_handlers);
}

void event_sock_write(event_sock_t *sock, event_handler_t *handler)
{
    assert(sock != NULL);
    if (handler == NULL) {
        return;
    }

    // attempt to write remaining bytes
#ifdef CONTIKI
    // do nothing if already sending
    if (handler->event.write.sending > 0) {
        return;
    }

    int len = MIN(cbuf_len(&handler->event.write.buf), uip_mss());
#else
    int len = cbuf_len(&handler->event.write.buf);
#endif
    uint8_t buf[len];
    cbuf_peek(&handler->event.write.buf, buf, len);

#ifdef CONTIKI
    // Send data
    uip_send(buf, len);

    // set sending length
    handler->event.write.sending = len;
#else
    int written = send(sock->descriptor, buf, len, MSG_DONTWAIT);
    if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        event_sock_write_notify(sock, handler, -1);
        return;
    }

    // remove written data from buffer
    cbuf_pop(&handler->event.write.buf, NULL, written);

    // notify of write when buffer is empty
    if (cbuf_len(&handler->event.write.buf) <= 0) {
        event_sock_write_notify(sock, handler, 0);
    }
#endif
}

#ifndef CONTIKI
void event_loop_io(event_loop_t *loop, int millis)
{
    assert(loop->nfds >= 0);

    fd_set read_fds = loop->active_fds;
    fd_set write_fds = loop->active_fds;
    struct timeval tv = { 0, millis * 1000 };

    // poll list of file descriptors with select
    if (select(loop->nfds, &read_fds, &write_fds, NULL, &tv) < 0) {
        // if an error different from interrupt is caught
        // here, there is an issue with the implementation
        assert(errno == EINTR);
        return;
    }

    for (int i = 0; i < loop->nfds; i++) {
        event_sock_t *sock;
        if (FD_ISSET(i, &loop->active_fds)) {
            sock = event_socket_find(loop->polling, i);
        }

        // check read operations
        if (FD_ISSET(i, &read_fds)) {
            if (sock->state == EVENT_SOCK_CONNECTED) {
                // read into sock buffer if any
                event_sock_read(sock, event_handler_find(sock->handlers, EVENT_READ_TYPE));
            }
            else {
                event_sock_connect(sock, event_handler_find(sock->handlers, EVENT_CONNECTION_TYPE));
            }
        }

        // check write operations
        if (FD_ISSET(i, &write_fds)) {
            // write from buffer if possible
            event_sock_write(sock, event_handler_find(sock->handlers, EVENT_WRITE_TYPE));
        }
    }
}

void event_loop_pending(event_loop_t *loop)
{
    for (event_sock_t *sock = loop->polling; sock != NULL; sock = sock->next) {
        event_handler_t *handler = event_handler_find(sock->handlers, EVENT_READ_TYPE);
        if (handler != NULL) {
            event_sock_read_notify(sock, handler);
        }
    }
}
#else
#ifdef EVENT_SOCK_POLL_TIMER
void event_poll_tcp(void *data)
{
    event_sock_t *sock = (event_sock_t *)data;

    // perform tcp poll
    tcpip_poll_tcp(sock->uip_conn);

    // reset timer
    ctimer_reset(&sock->timer);
}
#endif
void event_handle_ack(event_sock_t *sock, event_handler_t *handler)
{
    if (handler == NULL || handler->event.write.sending == 0) {
        return;
    }

    // remove sent data from output buffer
    cbuf_pop(&handler->event.write.buf, NULL, handler->event.write.sending);

    // update sending size
    handler->event.write.sending = 0;

    // notify of write when buffer is empty
    if (cbuf_len(&handler->event.write.buf) <= 0) {
        event_sock_write_notify(sock, handler, 0);
    }
    else {
        // perform write of remaining bytes
        event_sock_write(sock, handler);
    }
}

void event_handle_rexmit(event_sock_t *sock, event_handler_t *handler)
{
    if (handler == NULL || handler->event.write.sending == 0) {
        return;
    }

    // reset send counter
    handler->event.write.sending = 0;

    // try again
    event_sock_write(sock, handler);
}

void event_handle_tcp_event(event_loop_t *loop, void *data)
{
    event_sock_t *sock = (event_sock_t *)data;

    if (sock != NULL && sock->uip_conn != NULL && sock->uip_conn != uip_conn) {
        /* Safe-guard: this should not happen, as the incoming event relates to
         * a previous connection */
        DEBUG("If you are reading this, there is an implementation error");
        return;
    }

    if (uip_connected()) {
        if (sock == NULL) { // new client connection
            sock = LIST_FIND(loop->polling, \
                             LIST_ELEM(event_sock_t)->state == EVENT_SOCK_LISTENING && \
                             UIP_HTONS(LIST_ELEM(event_sock_t)->descriptor) == uip_conn->lport);

            // Save the connection for when
            // accept is called
            sock->uip_conn = uip_conn;

            // do connect
            event_sock_connect(sock, event_handler_find(sock->handlers, EVENT_CONNECTION_TYPE));
        }

        if (sock == NULL) { // no one waiting for the socket
            uip_abort();
        }
        else {
            if (uip_newdata()) {
                WARN("read new data but there is no socket ready to receive it yet");
            }
        }
        return;
    }

    // no socket for the connection
    // probably it was remotely closed
    if (sock == NULL) {
        uip_abort();
        return;
    }

    event_handler_t *rh = event_handler_find(sock->handlers, EVENT_READ_TYPE);
    event_handler_t *wh = event_handler_find(sock->handlers, EVENT_WRITE_TYPE);
    if (uip_timedout() || uip_aborted() || uip_closed()) { // Remote connection closed or timed out
        // if waiting for read close the read buffer
        if (rh != NULL) {
            // do not allow more data in read buffer
            cbuf_end(&rh->event.read.buf);
        }

        // Handle pending reads one last time
        event_sock_read_notify(sock, rh);

        // notify all pending write handlers
        while (wh != NULL) {
            event_sock_write_notify(sock, wh, -1);
            wh = event_handler_find(sock->handlers, EVENT_WRITE_TYPE);
        }

        // Finish connection
        uip_close();

        return;
    }
    
    if (uip_acked()) {
        event_handle_ack(sock, wh);

        // look for a new handler
        wh = event_handler_find(sock->handlers, EVENT_WRITE_TYPE);
    }

    if (uip_rexmit()) {
        event_handle_rexmit(sock, wh);
    }

    if (uip_newdata()) {
        event_sock_read(sock, rh);
    }

    // Handle pending reads and writes if available
    // if it is a poll event, it will jump to here
    event_sock_read_notify(sock, rh);
    event_sock_write(sock, wh);

    // Close connection cleanly
    if (sock->state == EVENT_SOCK_CLOSING && wh == NULL) {
        uip_close();
    }
}
#endif // CONTIKI

void event_loop_close(event_loop_t *loop)
{
    event_sock_t *curr = loop->polling;
    event_sock_t *prev = NULL;

#ifndef CONTIKI
    int max_fds = -1;
#endif

    while (curr != NULL) {
        // if the event is closing and we are not waiting to write
        event_handler_t *wh = event_handler_find(curr->handlers, EVENT_WRITE_TYPE);
        if (curr->state == EVENT_SOCK_CLOSING && (wh == NULL || cbuf_len(&wh->event.write.buf) <= 0)) {
#ifndef CONTIKI
            // prevent sock to be used in polling
            FD_CLR(curr->descriptor, &loop->active_fds);

            // notify the client of socket closing
            shutdown(curr->descriptor, SHUT_WR);

            // close the socket and update its status
            close(curr->descriptor);
#else
            // tcp_markconn()
            if (curr->uip_conn != NULL) {
                tcp_markconn(curr->uip_conn, NULL);
            }

            event_handler_t *ch = event_handler_find(curr->handlers, EVENT_CONNECTION_TYPE);
            if (ch != NULL) {
                // If server socket
                // let UIP know that we are no longer accepting connections on the specified port
                PROCESS_CONTEXT_BEGIN(&event_loop_process);
                tcp_unlisten(UIP_HTONS(curr->descriptor));
                PROCESS_CONTEXT_END();
            }
#ifdef EVENT_SOCK_POLL_TIMER
            else {
                // is a client
                ctimer_stop(&curr->timer);
            }
#endif
#endif
            curr->state = EVENT_SOCK_CLOSED;

            // call the close callback
            curr->close_cb(curr);

            // Move curr socket to unused list
            event_sock_t *next = LIST_PUSH(curr, loop->unused);

            // remove the socket from the polling list
            // and move the socket back to the unused list
            if (prev == NULL) {
                loop->polling = next;
            }
            else {
                prev->next = next;
            }

            // move socket handlers back to the unused handler list
            for (event_handler_t *h = curr->handlers; h != NULL; h = LIST_PUSH(h, loop->unused_handlers)) {}

            // move the head forward
            curr = next;
            continue;
        }

#ifndef CONTIKI
        if (curr->descriptor > max_fds) {
            max_fds = curr->descriptor;
        }
#endif

        prev = curr;
        curr = curr->next;
    }

#ifndef CONTIKI
    // update the max file descriptor
    loop->nfds = max_fds + 1;
#endif
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

#ifndef CONTIKI
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
#else
    // use port as the socket descriptor
    sock->descriptor = port;

    // Let know UIP that we are accepting connections on the specified port
    PROCESS_CONTEXT_BEGIN(&event_loop_process);
    tcp_listen(UIP_HTONS(port));
    PROCESS_CONTEXT_END();
#endif // CONTIKI

    // update sock state
    sock->state = EVENT_SOCK_LISTENING;

    // add socket to polling queue
    LIST_PUSH(sock, loop->polling);

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

    // see if there is already a read handler set
    event_handler_t *handler = event_handler_find(sock->handlers, EVENT_READ_TYPE);
    if (handler != NULL) {
        // update callback
        handler->event.read.cb = cb;
        return 0;
    }

    // find free handler
    event_loop_t *loop = sock->loop;
    handler = event_handler_find_free(loop, sock);
    assert(handler != NULL);

    // set handler event
    handler->type = EVENT_READ_TYPE;
    memset(&handler->event.read, 0, sizeof(event_read_t));

    // initialize read buffer
    cbuf_init(&handler->event.read.buf, handler->event.read.buf_data, EVENT_MAX_BUF_READ_SIZE);

    // set write callback
    handler->event.read.cb = cb;

    return 0;
}

void event_read_stop(event_sock_t *sock)
{
    assert(sock != NULL);
    assert(sock->loop != NULL);

    // see if there is already a read handler set
    event_handler_t *handler = event_handler_find(sock->handlers, EVENT_READ_TYPE);
    assert(handler != NULL);

    // remove callback
    handler->event.read.cb = NULL;
}

int event_write(event_sock_t *sock, unsigned int size, uint8_t *bytes, event_write_cb cb)
{
    // check socket status
    assert(sock != NULL);
    assert(sock->loop != NULL);

    // write can only be performed on a connected socket
    assert(sock->state == EVENT_SOCK_CONNECTED);
    assert(size < EVENT_MAX_BUF_WRITE_SIZE);

    // find free handler
    event_loop_t *loop = sock->loop;
    event_handler_t *handler = event_handler_find_free(loop, sock);
    if (handler == NULL) {
        ERROR("No more available handlers for event_write operation. Increase EVENT_MAX_HANDLERS macro");   // TODO: Borrar esta linea
    }
    assert(handler != NULL);                                                                                // no more handlers available

    // set handler event
    handler->type = EVENT_WRITE_TYPE;
    memset(&handler->event.write, 0, sizeof(event_write_t));

    // initialize write buffer
    cbuf_init(&handler->event.write.buf, handler->event.write.buf_data, EVENT_MAX_BUF_WRITE_SIZE);

    // set write callback
    handler->event.write.cb = cb;

    // write bytes to output buffer
    int to_write = cbuf_push(&handler->event.write.buf, bytes, size);

#ifdef CONTIKI
    tcpip_poll_tcp(sock->uip_conn);
#endif

    return to_write;
}

int event_read_stop_and_write(event_sock_t *sock, unsigned int size, uint8_t *bytes, event_write_cb cb)
{
    event_read_stop(sock);
    return event_write(sock, size, bytes, cb);
}

int event_accept(event_sock_t *server, event_sock_t *client)
{
    assert(server != NULL && client != NULL);
    assert(server->loop != NULL && client->loop != NULL);

    // check correct socket state
    assert(server->state == EVENT_SOCK_LISTENING);
    assert(client->state == EVENT_SOCK_CLOSED);

    event_loop_t *loop = client->loop;

#ifdef CONTIKI
    // Check that a new connection has been established
    assert(server->uip_conn != NULL);

    // Connection belongs to the client
    client->uip_conn = server->uip_conn;
    server->uip_conn = NULL;

    // attach socket state to uip_connection
    tcp_markconn(client->uip_conn, client);

    // use same port for client
    client->descriptor = server->descriptor;

#ifdef EVENT_SOCK_POLL_TIMER
    // set tcp poll timer every 10 ms
    ctimer_set(&client->timer, CLOCK_SECOND / EVENT_SOCK_POLL_TIMER_FREQ, event_poll_tcp, client);
#endif
#else
    int clifd = accept(server->descriptor, NULL, NULL);
    if (clifd < 0) {
        ERROR("Failed to accept new client");
        return -1;
    }

    // update max fds value
    loop->nfds = clifd + 1;

    // add socket fd to read watchlist
    FD_SET(clifd, &loop->active_fds);

    // set client variables
    client->descriptor = clifd;
#endif

    // set client state
    client->state = EVENT_SOCK_CONNECTED;

    // add socket to polling list
    LIST_PUSH(client, loop->polling);

    return 0;
}

int event_close(event_sock_t *sock, event_close_cb cb)
{
    // check socket status
    assert(sock != NULL);
    assert(sock->loop != NULL);
    assert(cb != NULL);

    // set sock state and callback
    sock->state = EVENT_SOCK_CLOSING;
    sock->close_cb = cb;

#ifdef CONTIKI
    // notify the loop process
    process_post(&event_loop_process, PROCESS_EVENT_CONTINUE, NULL);
#endif

    return 0;
}

void event_loop_init(event_loop_t *loop)
{
    assert(loop != NULL);
    // fail if loop pointer is not initialized

    // reset loop memory
    memset(loop, 0, sizeof(event_loop_t));

#ifndef CONTIKI
    // reset active file descriptor list
    FD_ZERO(&loop->active_fds);
#endif

    // reset socket memory
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
    event_sock_t *sock = LIST_POP(loop->unused);

    // reset sock memory
    memset(sock, 0, sizeof(event_sock_t));

    // assign sock variables
    sock->loop = loop;
    sock->state = EVENT_SOCK_CLOSED;

    return sock;
}

int event_sock_unused(event_loop_t *loop)
{
    assert(loop != NULL);

    return LIST_COUNT(loop->unused);
}

#ifdef CONTIKI
static event_loop_t *loop;

void event_loop(event_loop_t *l)
{
    assert(l != NULL);
    assert(loop == NULL);

    // set global variable
    loop = l;
    process_start(&event_loop_process, NULL);
}

PROCESS_THREAD(event_loop_process, ev, data)
#else
void event_loop(event_loop_t *loop)
#endif
{
#ifdef CONTIKI
    PROCESS_BEGIN();
#endif
    assert(loop != NULL);
    assert(loop->running == 0);

    loop->running = 1;
    while (event_loop_is_alive(loop)) {
#ifdef CONTIKI
        PROCESS_WAIT_EVENT();

        // todo: handle timer events

        if (ev == tcpip_event) {
            // handle network events
            event_handle_tcp_event(loop, data);
        }
#else
        // todo: perform timer events
        // perform I/O events with 0 timeout
        event_loop_io(loop, 0);

        // handle unprocessed read data
        event_loop_pending(loop);
#endif

        // perform close events
        event_loop_close(loop);
    }
    loop->running = 0;
#ifdef CONTIKI
    PROCESS_END();
#endif
}
