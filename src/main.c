/**
 * Basic tcp server using select()
 * Based on https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
 *
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "logging.h"
#include "event_loop.h"

#define PORT (8888)
#define MAX_BUF_SIZE (128)
#define MAX_NO_OF_CLIENTS (1)

typedef struct
{
    int sock;
    int is_used;
    event_handler_t handler;
} client_t;

typedef struct
{
    int sock;
    event_handler_t handler;

    client_t clients[MAX_NO_OF_CLIENTS];
} server_t;

static server_t server;

static int find_client(server_t *server, client_t *client)
{
    int client_slot = -1;

    for (int i = 0; i < MAX_NO_OF_CLIENTS && client_slot < 0; i++)
    {
        if (client->is_used == 1 && client == &server->clients[i])
        {
            client_slot = i;
        }
    }

    return client_slot;
}

static int find_free_slot(server_t *server)
{
    int slot = -1;
    for (int i = 0; i < MAX_NO_OF_CLIENTS && slot < 0; i++)
    {
        if (server->clients[i].is_used == 0)
        {
            slot = i;
        }
    }
    return slot;
}

int get_client_socket(void *instance)
{
    const client_t *client = instance;

    return client->sock;
}

int get_server_socket(void *instance)
{
    const server_t *server = instance;

    return server->sock;
}

void on_recv(void *instance)
{
    client_t *client = instance;
    int fd = client->sock;

    char buffer[MAX_BUF_SIZE];
    int nbytes;

    nbytes = read(fd, buffer, MAX_BUF_SIZE);
    if (nbytes < 0)
    {
        /* Read error. */
        ERROR("In read(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    else if (nbytes == 0)
    {
        // close socket and unregister
        server_t *s = &server;
        int slot = find_client(s, client);

        if (slot >= 0)
        {
            s->clients[slot].is_used = 0;
            unregister_handler(&client->handler);
        }
        INFO("Client disconnected\n");
        return;
    }
    else
    {
        /* Data read. */
        INFO("Received message: '%.*s'\n", nbytes - 1, buffer);
    }
}

void create_client(int sock, client_t *client)
{
    event_handler_t h = {.instance = client, .get_fd = get_client_socket, .handle = on_recv};
    client->sock = sock;
    client->handler = h;
    client->is_used = 1;
}

void on_new_client(void *instance)
{
    server_t *server = instance;
    event_handler_t *handler = &server->handler;

    assert(handler != NULL);

    // Setup address
    char addr_str[INET6_ADDRSTRLEN];
    struct sockaddr_in6 addr;
    socklen_t addr_size = sizeof(addr);

    int new = accept(handler->get_fd(server),
                     (struct sockaddr *)&addr,
                     &addr_size);
    if (new < 0)
    {
        ERROR("In accept(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    inet_ntop(AF_INET6, &addr.sin6_addr, addr_str, sizeof(addr_str));
    INFO("Received connection from host [%s]:%u\n",
         addr_str,
         addr.sin6_port);

    // Check if number of client connections has been reached
    int slot = find_free_slot(server);
    if (slot >= 0)
    {
        client_t *client = &server->clients[slot];
        create_client(new, client);

        // Register the client handler
        register_handler(&client->handler);

        INFO("Connection from host [%s]:%u accepted.\n", addr_str, addr.sin6_port);
    }
    else
    {
        // Reject new connection
        close(new);
        WARN("Maximum number of clients reached. Connection from host [%s]:%u rejected.\n", addr_str, addr.sin6_port);
    }
}

server_t *create_server(uint16_t port)
{
    int sock = -1, option = 1;
    struct sockaddr_in6 addr;

    /* Create the socket. */
    sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0)
    {
        ERROR("In socket(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Give the socket a addr. */
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    memset(&addr.sin6_addr, 0, sizeof(addr.sin6_addr));

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        ERROR("In bind(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* avoid waiting port close time to use server immediately */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option)) < 0)
    {
        ERROR("In setsockopt(): %s\n", strerror(errno));
        close(sock);
        exit(EXIT_FAILURE); 
    }

    server_t *s = &server;
    event_handler_t h = {.instance = s, .get_fd = get_server_socket, .handle = on_new_client};
    s->sock = sock;
    s->handler = h;

    return s;
}

void server_listen(server_t *server)
{
    if (listen(server->sock, MAX_NO_OF_CLIENTS) < 0)
    {
        ERROR("In listen(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    /* Create the socket and set it up to accept connections. */
    server_t *s = create_server(PORT);

    // Register server handler
    register_handler(&s->handler);

    // Start listening for events
    server_listen(s);

    INFO("Server started, waiting for connections ...\n");
    // Start event-loop
    handle_events();
}
