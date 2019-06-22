#include <stdio.h>
#include <errno.h>

/* Contiki dependencies */
#include "contiki-net.h"
#include "lib/list.h"
#include "lib/memb.h"

/* Local dependencies */
#include "sock.h"
#include "cbuf.h"

#define ENABLE_DEBUG 1
#include "logging.h"

enum {
    SOCKET_FLAGS_NONE = 0x00,
    SOCKET_FLAGS_LISTENING = 0x01,
    SOCKET_FLAGS_SENDING = 0x02,
    SOCKET_FLAGS_CLOSING = 0x04
};

struct sock_socket {
    // Socket buffers
    uint8_t in[SOCK_BUFFER_SIZE];
    uint8_t out[SOCK_BUFFER_SIZE];
   
    // circular buffers to manage data
    cbuf_t cin;
    cbuf_t cout;

    // last send length
    uint16_t last_send;

    // listen port
    uint16_t port;

    // socket state
    uint8_t flags;
    
    struct process * process;
    struct sock_socket * next;
};

MEMB(sockets, struct sock_socket, SOCK_MAX_SOCKETS);
LIST(socket_list);

// Define sock process
PROCESS(sock_process, "TCP sockets");

static struct sock_socket * find_socket_for_port(uint16_t port) {
    struct sock_socket * s;
    for (s = list_head(socket_list); s != NULL; s = list_item_next(s)) {
        if (s->port == port) {
            return s;
        }
    }
    return NULL;
}

static void sock_init(void)
{
    static uint8_t inited = 0;

    if (!inited) {
        memb_init(&sockets);
        list_init(socket_list);
        process_start(&sock_process, NULL);
        inited = 1;
    }
}


int sock_create(sock_t *sock)
{
    if (sock == NULL || sock->socket != NULL) {
        errno = EINVAL;
        return -1;
    }

    // Initialize socket library
    sock_init();

    sock->port = 0;
    sock->state = SOCK_OPENED;

    
    return 0;
}

int sock_listen(sock_t *sock, uint16_t port)
{
    if ((sock == NULL) || (sock->state != SOCK_OPENED)) {
        errno = EINVAL;
        return -1;
    }

    if (sock->socket != NULL) {
        errno = EBADF;
        return -1;
    }

    if (find_socket_for_port(port) != NULL) {
        errno = EADDRINUSE;
        return -1;
    }
   
    // Check if there is enough memory available
    if (memb_numfree(&sockets) <= 0) {
        errno = ENOMEM;
        return -1;
    }

    // ALlocate memory for socket
    struct sock_socket * s = memb_alloc(&sockets);
    
    // Initialize input and output buffers
    cbuf_init(&s->cin, s->in, sizeof(s->in));
    cbuf_init(&s->cout, s->out, sizeof(s->out));

    s->port = port;
    s->process = PROCESS_CURRENT();
    s->flags = SOCKET_FLAGS_NONE;

    // add socket to the list for later retrieval
    list_add(socket_list, s);

    // Let know UIP that we are accepting connections on the specified port
    PROCESS_CONTEXT_BEGIN(&sock_process);
    tcp_listen(UIP_HTONS(port));
    PROCESS_CONTEXT_END();

    sock->port = port;
    sock->socket->flags |= SOCKET_FLAGS_LISTENING;
    sock->state = SOCK_LISTENING;
    return 0;
}


int sock_accept(sock_t *server, sock_t *client)
{
    if ((server == NULL)) {
        errno = EINVAL;
        return -1;
    }

    if ((server->socket == NULL)) {
        errno = ENOTSOCK;
        return -1;
    }

    if ((server->socket->flags & SOCKET_FLAGS_LISTENING) != 0)  {
        errno = ENOTCONN;
        return -1;
    }

    // Only struct values if client is not null
    if (client != NULL) {
        client->socket = server->socket;
        client->state = SOCK_CONNECTED;
    }

    return 0;
}

int sock_connect(sock_t * client, char * addr, uint16_t port) {
    return -1;
}


int sock_read(sock_t *sock, char *buf, int len, int timeout) 
{
	if (sock == NULL || (sock->state != SOCK_CONNECTED)) {
		errno = EINVAL;
		return -1;
	}

	if (sock->socket == NULL) {
		errno = ENOTSOCK;
		return -1;
	}

	if (buf == NULL) {
		errno = EINVAL;
		return -1;
	}

	// Read from readptr into buf
    return cbuf_pop(&sock->socket->cin, buf, len);
}

int sock_write(sock_t * sock, char * buf, int len) {
    if (sock == NULL || (sock->state != SOCK_CONNECTED)) {
		errno = EINVAL;
		return -1;
	}

	if (sock->socket == NULL) {
		errno = ENOTSOCK;
		return -1;
	}

	if (buf == NULL) {
		errno = EINVAL;
		return -1;
	}

    return cbuf_push(&sock->socket->cout, buf, len);
}

int sock_destroy(sock_t *sock)
{
    if (sock == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (sock->socket == NULL || sock->state == SOCK_CLOSED) {
        errno = EBADF;
        return -1;
    }

    // Free the memory
    memb_free(&sockets, sock->socket); 

    sock->socket = NULL;
    sock->state = SOCK_CLOSED;
    return 0;
}

PT_THREAD(sock_wait_client(sock_t * sock)) 
{
    PT_BEGIN(&sock->pt);

    if (sock == NULL) {
        errno = EINVAL;
        return -1;
    }
    
    if (sock->socket == NULL) {
        errno = ENOTSOCK;
        return -1;
    }

    PT_WAIT_WHILE(&sock->pt, (sock->socket->flags & SOCKET_FLAGS_LISTENING) != 0);
       
    PT_END(&sock->pt);
}

PT_THREAD(sock_wait_data(sock_t * sock))
{
    PT_BEGIN(&sock->pt);
    
    if (sock == NULL) {
        errno = EINVAL;
        return -1;
    }
    
    if (sock->socket == NULL) {
        errno = ENOTSOCK;
        return -1;
    }

    // Wait while there is no data in the socket or server
    PT_WAIT_WHILE(&sock->pt, cbuf_len(&sock->socket->cin) == 0 && (sock->socket->flags & SOCKET_FLAGS_LISTENING) == 0);

    PT_END(&sock->pt);
}

void send_data(struct sock_socket * s) 
{
    if (s == NULL || s->flags & SOCKET_FLAGS_SENDING || cbuf_len(&s->cout) == 0) return;

    int len = MIN(cbuf_len(&s->cout), uip_mss());
    uint8_t buf[len];    

    // Copy data to send buffer
    cbuf_peek(&s->cout, buf, len);

    // Send data
    uip_send(buf, len);

    s->flags |= SOCKET_FLAGS_SENDING;
    s->last_send = len;
}

void handle_ack(struct sock_socket * s) {
    if (s == NULL || (s->flags & SOCKET_FLAGS_SENDING) == 0)  return;

    // remove data from send buffer
    cbuf_pop(&s->cout, NULL, s->last_send);

    s->flags &= ~SOCKET_FLAGS_SENDING;

    // send next data
    send_data(s);
}

void handle_rexmit(struct sock_socket * s) {
    if (s == NULL || (s->flags & SOCKET_FLAGS_SENDING) == 0)  return;
    
    s->flags &= ~SOCKET_FLAGS_SENDING;
    
    // re-send data
    send_data(s);
}

void handle_tcp_event(void *state)
{
    struct sock_socket *s = (struct sock_socket *)state;

    if (uip_connected()) {
        if (s == NULL) { // new client connection
            // find someone waiting for this socket
            for (s = list_head(socket_list); s != NULL; s = list_item_next(s)) {
                if ((s->flags & SOCKET_FLAGS_LISTENING) != 0 && uip_conn->lport == UIP_HTONS(s->port)) {
                    // Set socket state to connected
                    s->flags &= ~SOCKET_FLAGS_LISTENING;

                    // Attach socket state to uip_connection
                    tcp_markconn(uip_conn, s);
                    break;
                }
            }
        }
        else { // connected to remote server
        }

        if (s == NULL) { // no one waiting for the socket
            uip_abort();
        }
        else {
            if (uip_newdata()) {
                cbuf_push(&s->cin, uip_appdata, uip_datalen());
            }
            send_data(s);
           
            // Notify the process of a new event
            process_post(s->process, PROCESS_EVENT_CONTINUE, NULL);
        }
        return;
    }

    // no socket for the connection
    if (s == NULL) {
        uip_abort();
        return;
    }

    if (uip_timedout() || uip_aborted() || uip_closed()) { // Connection timed out
        s->flags |= SOCKET_FLAGS_LISTENING;

        tcp_markconn(uip_conn, NULL);

        // Notify the original process of the event
        process_post(s->process, PROCESS_EVENT_CONTINUE, NULL);

        return;
    }

    if (uip_newdata()) {
        cbuf_push(&s->cin, uip_appdata, uip_datalen());
    }

    if (uip_acked()) {
        handle_ack(s);
    }

    if (uip_rexmit()) {
        handle_rexmit(s);
    }

    if (uip_newdata() || uip_poll()) {
        send_data(s);
    }

    // Notify the original process of the event
    process_post(s->process, PROCESS_EVENT_CONTINUE, NULL);
}


/* Configure sock process */
PROCESS_THREAD(sock_process, ev, data){
    PROCESS_BEGIN();

    while (1) {
        // Wait untill we receive a TCP event
        PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

        handle_tcp_event(data);
    }
    PROCESS_END();
}
