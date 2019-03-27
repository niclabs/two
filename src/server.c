#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "context.h"
#include "server.h"
#include "logging.h"
#include "event_loop.h"

#define MAX_BUF_SIZE (256)
#define MAX_NO_OF_CLIENTS (1)

typedef struct client {
    int is_used;
    client_ctx_t ctx;
    event_handler_t handler;
} client_t;

struct server
{
    int fd;
    int created;
    event_handler_t handler;
    client_t clients[MAX_NO_OF_CLIENTS];
};

server_t server_g;

/**
 * Utility function to find a free slot in the server client list
 */
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

/**
 * Perform cleanup operations on client
 */
static void client_destroy(client_t * client) {
    assert(client != NULL);

    unregister_handler(&client->handler);
    client_ctx_destroy(&client->ctx);
    client->is_used = 0;

    INFO("Client disconnected\n");
}

/**
 * get_fd callback for client event handler
 */
static int get_client_socket(void *instance)
{
    const client_t *client = instance;
    return client->ctx.fd;
}

/**
 * handle_event callback for client event handler\
 */
static void on_client_receive(void *instance)
{
    client_t *client = instance;
    int fd = client->ctx.fd;

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
        client_destroy(client);
        return;
    }
    else
    {
        /* Data read. */
        INFO("Received message: '%.*s'\n", nbytes - 1, buffer);
    }
}

/**
 * Initialize client for sock file descriptor
 */
static void client_init(int sock, client_t *client)
{
    // Initialize client context
    client_ctx_t ctx;
    client_ctx_init(sock, &ctx);

    // Initialize event handler
    event_handler_t h = {.instance = client, .get_fd = get_client_socket, .handle = on_client_receive};

    // Set client parameters
    client->is_used = 1;
    client->ctx = ctx;
    client->handler = h;
}

/**
 * get_fd callback for server event handler
 */
static int get_server_socket(void *instance)
{
    const server_t *server = instance;
    return server->fd;
}

/**
 * handle_event callbak for server event handler
 */
static void on_new_client(void *instance)
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
        // Create the new client
        client_t *client = &server->clients[slot];
        client_init(new, client);

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


server_t *server_create(uint16_t port)
{
    assert(server_g.created == 0);

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

    server_t *s = &server_g;
    event_handler_t h = {.instance = s, .get_fd = get_server_socket, .handle = on_new_client};
    s->fd = sock;
    s->handler = h;
    s->created = 1;

    return s;
}

void server_listen(server_t * server) {
    assert(server != NULL);
    assert(server->created == 1);

    // Register server handler
    register_handler(&server->handler);

    if (listen(server->fd, MAX_NO_OF_CLIENTS) < 0)
    {
        ERROR("In listen(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    INFO("Server started, waiting for connections ...\n");

    // Start event-loop
    handle_events();
}

void server_destroy(server_t * server) {
    // Destroy all clients
    // TODO: what if there are pending operations?
    for (int i = 0; i < MAX_NO_OF_CLIENTS; i++) {
        if (server->clients[i].is_used != 0) {
            client_destroy(&server->clients[i]);
        }
    }

    // Unregister server handler
    unregister_handler(&server->handler);

    // Close server socket
    close(server->fd);
    INFO("Server stopped successfully.\n");
}