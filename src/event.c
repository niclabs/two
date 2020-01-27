#include <errno.h>
#include <assert.h>

#ifndef CONTIKI
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "event.h"

#define LOG_MODULE LOG_MODULE_EVENT
#include "logging.h"

#ifdef CONTIKI
// Main contiki process
PROCESS(event_loop_process, "Event loop process");
#endif

/////////////////////////////////////////////////////
// Private methods
/////////////////////////////////////////////////////

// find socket file descriptor in socket queue
event_t *event_find(event_t *queue, event_type_t type)
{
    return LL_FIND(queue, LL_ELEM(event_t)->type == type);
}

// find free event in loop memory
event_t *event_find_free(event_loop_t *loop, event_sock_t *sock)
{
    return LL_MOVEP(loop->events, sock->events);
}

// find socket file descriptor in socket queue
event_sock_t *event_sock_find(event_sock_t *queue, event_descriptor_t descriptor)
{
    return LL_FIND(queue, LL_ELEM(event_sock_t)->descriptor == descriptor);
}

event_sock_t * event_sock_connect(event_sock_t *sock, event_t *event)
{
    if (event != NULL) {
        // if this happens there is an error with the implementation
        assert(event->data.connection.cb != NULL);

        // only notify of new connection if there 
        // are sockets available to receive it
        if (sock->loop->sockets != NULL) {
            return event->data.connection.cb(sock);
        }
    }
    return NULL;
}

void event_sock_close(event_sock_t *sock, int status)
{
    assert(status <= 0);
    event_t *re = event_find(sock->events, EVENT_READ_TYPE);
    if (re != NULL) {
        // mark the buffer as closed
        cbuf_end(&re->data.read.buf);

        // notify the read event
        re->data.read.cb(sock, status, NULL);
    }

    event_t *we = event_find(sock->events, EVENT_WRITE_TYPE);
    if (we != NULL) {
        // mark the buffer as closed
        cbuf_end(&we->data.write.buf);

        // empty the buffer
        cbuf_pop(&we->data.write.buf, NULL, cbuf_len(&we->data.write.buf));

        event_write_op_t *op = LL_POP(we->data.write.queue);
        // notify of error if we could not notify the read
        if (re == NULL && op != NULL) {
            op->cb(sock, status);
        }

        // remove all pending operations
        while (op != NULL) {
            LL_PUSH(op, sock->loop->writes);
            op = LL_POP(we->data.write.queue);
        }
    }
}

void event_sock_read(event_sock_t *sock, event_t *event)
{
    assert(sock != NULL);
    if (event == NULL) {
        return;
    }

    // check available buffer data
    int readlen = cbuf_maxlen(&event->data.read.buf) - cbuf_len(&event->data.read.buf);
    uint8_t buf[readlen];

#ifdef CONTIKI
    int count = MIN(readlen, uip_datalen());
    memcpy(buf, uip_appdata, count);
#else
    // perform read in non blocking manner
    int count = recv(sock->descriptor, buf, readlen, MSG_DONTWAIT);
    if ((count < 0 && errno != EAGAIN && errno != EWOULDBLOCK) || count == 0) {
        event_sock_close(sock, count == 0 ? 0 : -errno);
        return;
    }
#endif
    DEBUG("received %d bytes from remote endpoint", count);
    // push data into buffer
    cbuf_push(&event->data.read.buf, buf, count);
}

void event_sock_handle_read(event_sock_t *sock, event_t *event)
{
    assert(sock != NULL);
    if (event == NULL) {
        return;
    }

    int buflen = cbuf_len(&event->data.read.buf);
    if (buflen > 0) {
        // else notify about the new data
        uint8_t read_buf[buflen];
        cbuf_peek(&event->data.read.buf, read_buf, buflen);

        DEBUG("passing %d bytes to callback", buflen);
        int readlen = event->data.read.cb(sock, buflen, read_buf);
        DEBUG("callback used %d bytes", readlen);

        // remove the read bytes from the buffer
        cbuf_pop(&event->data.read.buf, NULL, readlen);
    }

    if (cbuf_has_ended(&event->data.read.buf)) {
        // if a read terminated call the callback with -1
        event->data.read.cb(sock, -1, NULL);
    }
}

void event_sock_handle_write(event_sock_t *sock, event_t *event, unsigned int written)
{
    // notify the waiting write operatinons
    event_write_op_t *op = LL_POP(event->data.write.queue);

    while (op != NULL && written > 0) {
        if (op->bytes <= written) {
            // if all bytes have been read
            written -= op->bytes;

            DEBUG("wrote %d bytes, notifying callback", op->bytes);

            // notify the callback
            op->cb(sock, 0);

            // free the memory
            LL_PUSH(op, sock->loop->writes);
        }
        else {
            // if not enough bytes have been written yet
            // reduce the size and push the operation back to
            // the list
            op->bytes -= written;
            break;
        }

        op = LL_POP(event->data.write.queue);
    }

    if (op != NULL) {
        LL_PUSH(op, event->data.write.queue);
    }
}

void event_sock_write(event_sock_t *sock, event_t *event)
{
    assert(sock != NULL);
    if (event == NULL) {
        return;
    }

    // attempt to write remaining bytes
#ifdef CONTIKI
    // do nothing if already sending
    if (event->data.write.sending > 0) {
        return;
    }

    int len = MIN(cbuf_len(&event->data.write.buf), uip_mss());
#else
    int len = cbuf_len(&event->data.write.buf);
#endif
    uint8_t buf[len];
    cbuf_peek(&event->data.write.buf, buf, len);

#ifdef CONTIKI
    // Send data
    uip_send(buf, len);
    //DEBUG("trying to send %d bytes", len);

    if (len > 0) {
        DEBUG("sending %d bytes, waiting for ack", len);
    }

    // set sending length
    event->data.write.sending = len;
#else
    int written = send(sock->descriptor, buf, len, MSG_DONTWAIT);
    if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        event_sock_close(sock, -errno);
        return;
    }

    if (written > 0) {
        // remove written data from buffer
        cbuf_pop(&event->data.write.buf, NULL, written);

        // notify waiting callbacks
        event_sock_handle_write(sock, event, written);
    }
#endif
}

#ifndef CONTIKI
void event_loop_timers(event_loop_t *loop)
{
    // get current time
    struct timeval now;

    gettimeofday(&now, NULL);

    // for each socket and event, check if elapsed time has been reached
    for (event_sock_t *sock = loop->polling; sock != NULL; sock = sock->next) {
        event_t *event = sock->events;
        while (event != NULL) {
            if (event->type != EVENT_TIMER_TYPE) {
                event = event->next;
                continue;
            }

            struct timeval diff;

            // Get time difference
            timersub(&now, &event->data.timer.start, &diff);

            // time has finished
            if ((diff.tv_sec * 1000 + diff.tv_usec / 1000) >= event->data.timer.millis) {
                // run the callback and reset timer
                int remove = event->data.timer.cb(sock);
                event->data.timer.start = now;
                if (remove > 0) {
                    // remove event from list
                    LL_DELETE(event, sock->events);

                    // move event to loop unused list
                    LL_PUSH(event, loop->events);
                }
            }
            event = event->next;
        }
    }
}

void event_loop_poll(event_loop_t *loop, int millis)
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
            sock = event_sock_find(loop->polling, i);
        }
        else {
            continue;
        }

        // check read operations
        if (FD_ISSET(i, &read_fds)) {
            if (sock->state == EVENT_SOCK_CONNECTED) {
                // read into sock buffer if any
                event_sock_read(sock, event_find(sock->events, EVENT_READ_TYPE));
            }
            else {
                event_sock_connect(sock, event_find(sock->events, EVENT_CONNECTION_TYPE));
            }
        }

        // check write operations
        if (FD_ISSET(i, &write_fds)) {
            // write from buffer if possible
            event_sock_write(sock, event_find(sock->events, EVENT_WRITE_TYPE));
        }
    }
}

void event_loop_pending(event_loop_t *loop)
{
    for (event_sock_t *sock = loop->polling; sock != NULL; sock = sock->next) {
        event_t *event = event_find(sock->events, EVENT_READ_TYPE);
        if (event != NULL) {
            event_sock_handle_read(sock, event);
        }
    }
}
#else
void event_sock_handle_timer(void *data)
{
    event_t *event = (event_t *)data;
    event_sock_t *sock = event->sock;

    DEBUG("timer expired, notifying callback");

    // run the callback
    int remove = event->data.timer.cb(sock);

    if (remove > 0) {
        // stop ctimer
        ctimer_stop(&event->data.timer.ctimer);

        // remove event from list
        LL_DELETE(event, sock->events);

        // move event to loop unused list
        LL_PUSH(event, sock->loop->events);
    }
    else {
        ctimer_restart(&event->data.timer.ctimer);
    }
}

void event_sock_handle_ack(event_sock_t *sock, event_t *event)
{
    if (event == NULL || event->data.write.sending == 0) {
        return;
    }


    DEBUG("received ack for %d bytes", event->data.write.sending);

    // remove sent data from output buffer
    cbuf_pop(&event->data.write.buf, NULL, event->data.write.sending);

    //DEBUG("wrote %d bytes", event->data.write.sending);

    // notify the operations of successful write
    event_sock_handle_write(sock, event, event->data.write.sending);

    // update sending size
    event->data.write.sending = 0;
}

void event_sock_handle_rexmit(event_sock_t *sock, event_t *event)
{
    if (event == NULL || event->data.write.sending == 0) {
        return;
    }

    // reset send counter
    event->data.write.sending = 0;
}

void event_sock_handle_event(event_loop_t *loop, void *data)
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
            DEBUG("new client connection");
            event_sock_t * server = LL_FIND(loop->polling, \
                           LL_ELEM(event_sock_t)->state == EVENT_SOCK_LISTENING && \
                           UIP_HTONS(LL_ELEM(event_sock_t)->descriptor) == uip_conn->lport);

            // Save the connection for when
            // accept is called
            server->uip_conn = uip_conn;

            // do connect
            sock = event_sock_connect(server, event_find(server->events, EVENT_CONNECTION_TYPE));
        }

        if (sock == NULL) { // no one accepted the socket
            uip_abort();
        }
        else {
            if (uip_newdata()) {
                event_t *re = event_find(sock->events, EVENT_READ_TYPE);
                event_sock_read(sock, re);
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

    if (uip_timedout() || uip_aborted() || uip_closed()) { // Remote connection closed or timed out
        // perform close operations
        event_sock_close(sock, -1);

        // Finish connection
        uip_close();

        return;
    }

    event_t *we = event_find(sock->events, EVENT_WRITE_TYPE);
    if (uip_acked()) {
        event_sock_handle_ack(sock, we);
    }

    if (uip_rexmit()) {
        event_sock_handle_rexmit(sock, we);
    }

    event_t *re = event_find(sock->events, EVENT_READ_TYPE);
    if (uip_newdata()) {
        event_sock_read(sock, re);
    }

    // Handle pending reads and writes if available
    // if it is a poll event, it will jump to here
    event_sock_handle_read(sock, re);

    // Try to write each time
    event_sock_write(sock, we);

    // Close connection cleanly
    if (sock->state == EVENT_SOCK_CLOSING && (we == NULL || we->data.write.queue == NULL)) {
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
        event_t *we = event_find(curr->events, EVENT_WRITE_TYPE);
        if (curr->state == EVENT_SOCK_CLOSING && (we == NULL || we->data.write.queue == NULL)) {
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

            // stop timer if any
            event_t *timer = event_find(curr->events, EVENT_TIMER_TYPE);
            if (timer != NULL) {
                ctimer_stop(&timer->data.timer.ctimer);
            }

            event_t *ch = event_find(curr->events, EVENT_CONNECTION_TYPE);
            if (ch != NULL) {
                // If server socket
                // let UIP know that we are no longer accepting connections on the specified port
                PROCESS_CONTEXT_BEGIN(&event_loop_process);
                tcp_unlisten(UIP_HTONS(curr->descriptor));
                PROCESS_CONTEXT_END();
            }
#endif
            curr->state = EVENT_SOCK_CLOSED;

            // call the close callback
            curr->close_cb(curr);

            // Move curr socket to unused list
            event_sock_t *next = LL_PUSH(curr, loop->sockets);

            // remove the socket from the polling list
            // and move the socket back to the unused list
            if (prev == NULL) {
                loop->polling = next;
            }
            else {
                prev->next = next;
            }

            // move socket events back to the unused event list
            for (event_t *h = curr->events; h != NULL; h = LL_PUSH(h, loop->events)) {}

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
    LL_PUSH(sock, loop->polling);

    // add event event to socket
    event_t *event = event_find_free(loop, sock);
    assert(event != NULL);

    // set event event
    event->type = EVENT_CONNECTION_TYPE;
    event->sock = sock;
    memset(&event->data.connection, 0, sizeof(event_connection_t));
    event->data.connection.cb = cb;

    return 0;
}

void event_read_start(event_sock_t *sock, uint8_t *buf, unsigned int bufsize, event_read_cb cb)
{
    // check socket status
    assert(sock != NULL);
    assert(sock->loop != NULL);
    assert(buf != NULL);
    assert(cb != NULL);

    // read can only be performed on connected sockets
    assert(sock->state == EVENT_SOCK_CONNECTED);

    event_t *event = event_find(sock->events, EVENT_READ_TYPE);
    assert(event == NULL); // read stop must be called before call to event_read_start

    // find free event
    event_loop_t *loop = sock->loop;
    event = event_find_free(loop, sock);
    assert(event != NULL); // should we return -1 instead?

    // set event event
    event->type = EVENT_READ_TYPE;
    event->sock = sock;
    memset(&event->data.read, 0, sizeof(event_read_t));

    // initialize read buffer
    cbuf_init(&event->data.read.buf, buf, bufsize);

    // set read callback
    event->data.read.cb = cb;
}

int event_read(event_sock_t *sock, event_read_cb cb)
{
    // check socket status
    assert(sock != NULL);
    assert(sock->loop != NULL);
    assert(cb != NULL);

    // read can only be performed on connected sockets
    assert(sock->state == EVENT_SOCK_CONNECTED);

    // see if there is already a read event set
    event_t *event = event_find(sock->events, EVENT_READ_TYPE);
    assert(event != NULL); // event_read_start() must be called before

    // Update read callback
    event->data.read.cb = cb;

    return 0;
}

int event_read_stop(event_sock_t *sock)
{
    // check socket status
    assert(sock != NULL);
    assert(sock->loop != NULL);

    event_t *event = event_find(sock->events, EVENT_READ_TYPE);
    if (event == NULL) {
        return 0;
    }

    // remove read event from list
    LL_DELETE(event, sock->events);

    // reset read callback
    event->data.read.cb = NULL;

    // Move data to the beginning of the buffer
    int len = cbuf_len(&event->data.read.buf);
    uint8_t buf[len];
    cbuf_pop(&event->data.read.buf, buf, len);

    // Copy memory back to cbuf pointer
    memcpy(event->data.read.buf.ptr, buf, len);

    // move event to loop unused list
    LL_PUSH(event, sock->loop->events);

    return len;
}

void event_write_enable(event_sock_t *sock, uint8_t *buf, unsigned int bufsize)
{
    // check socket status
    assert(sock != NULL);
    assert(sock->loop != NULL);
    assert(bufsize > 0);
    assert(buf != NULL);

    // write can only be performed on a connected socket
    assert(sock->state == EVENT_SOCK_CONNECTED);

    // see if there is writing is already enabled
    event_t *event = event_find(sock->events, EVENT_WRITE_TYPE);

    // only one write event is allowed per socket
    assert(event == NULL);

    // find free event
    event_loop_t *loop = sock->loop;
    event = event_find_free(loop, sock);

    // If this fails, you need to increase the value of EVENT_MAX_EVENTS
    assert(event != NULL);

    // set event event and error callback
    event->type = EVENT_WRITE_TYPE;
    event->sock = sock;

    // initialize write buffer
    cbuf_init(&event->data.write.buf, buf, bufsize);
}

int event_write(event_sock_t *sock, unsigned int size, uint8_t *bytes, event_write_cb cb)
{
    // check socket status
    assert(sock != NULL);
    assert(sock->loop != NULL);
    assert(bytes != NULL);
    assert(cb != NULL);

    // write can only be performed on a connected socket
    assert(sock->state == EVENT_SOCK_CONNECTED);

    // find write event
    event_loop_t *loop = sock->loop;
    event_t *event = event_find(sock->events, EVENT_WRITE_TYPE);

    // this will fail if event_write is called before event_write_enable
    assert(event != NULL);

    // write bytes to output buffer
    int to_write = cbuf_push(&event->data.write.buf, bytes, size);
    DEBUG("queued %d bytes for writing", to_write);

    // add a write operation to the event
    if (to_write > 0) {
        event_write_op_t *op = LL_MOVE(loop->writes, event->data.write.queue);

        // If this fails increase CONFIG_EVENT_WRITE_QUEUE_SIZE
        assert(op != NULL);

        op->cb = cb;
        op->bytes = to_write;
    }

    // get free operation from loop
#ifdef CONTIKI
    // poll the socket to perform write
    tcpip_poll_tcp(sock->uip_conn);
#endif

    return to_write;
}

event_t *event_timer(event_sock_t *sock, unsigned int millis, event_timer_cb cb)
{
    // check socket status
    assert(sock != NULL);
    assert(sock->loop != NULL);
    assert(cb != NULL);

    // 0 == infinite
    if (millis == 0) {
        return NULL;
    }

    // Get a new event
    event_t *event = event_find_free(sock->loop, sock);
    assert(event != NULL);

    event->type = EVENT_TIMER_TYPE;
    event->sock = sock;
    event->data.timer.cb = cb;

#ifndef CONTIKI
    event->data.timer.millis = millis;
    // get start time
    gettimeofday(&event->data.timer.start, NULL);
#else
    ctimer_set(&event->data.timer.ctimer, (millis * CLOCK_SECOND) / 1000, event_sock_handle_timer, event);
#endif

    return event;
}

void event_timer_reset(event_t *timer)
{
    if (timer == NULL) {
        return;
    }
    assert(timer->type == EVENT_TIMER_TYPE);
#ifdef CONTIKI
    ctimer_restart(&timer->data.timer.ctimer);
#else
    gettimeofday(&timer->data.timer.start, NULL);
#endif
}

void event_timer_stop(event_t *timer)
{
    if (timer == NULL) {
        return;
    }
    assert(timer->type == EVENT_TIMER_TYPE);
    assert(timer->sock != NULL);
    event_sock_t *sock = timer->sock;

#ifdef CONTIKI
    // stop ctimer
    ctimer_stop(&timer->data.timer.ctimer);
#endif
    // remove event from list
    LL_DELETE(timer, sock->events);

    // move event to loop unused list
    LL_PUSH(timer, sock->loop->events);
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
    LL_PUSH(client, loop->polling);

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

    // find write event
    event_t *event = event_find(sock->events, EVENT_WRITE_TYPE);
    if (event != NULL) { // mark the buffer as closed
        cbuf_end(&event->data.write.buf);
    }

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
    LL_INIT(loop->sockets, EVENT_MAX_SOCKETS);
    LL_INIT(loop->events, EVENT_MAX_EVENTS);
    LL_INIT(loop->writes, EVENT_WRITE_QUEUE_SIZE);
}

event_sock_t *event_sock_create(event_loop_t *loop)
{
    assert(loop != NULL);

    // check that the loop is initialized
    // and there are available clients
    assert(loop->sockets != NULL);

    // get the first unused sock
    // and remove sock from unsed list
    event_sock_t *sock = LL_POP(loop->sockets);

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
    return LL_COUNT(loop->sockets);
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
#else
    // prevent send() raising a SIGPIPE ween
    // remote connection has closed
    signal(SIGPIPE, SIG_IGN);
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
            event_sock_handle_event(loop, data);
        }
#else
        // todo: perform timer events
        event_loop_timers(loop);

        // perform I/O events with 0 timeout
        event_loop_poll(loop, 0);

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
