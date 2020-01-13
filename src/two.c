/*
   This API contains the "dos" methods to be used by
   HTTP/2
 */

#include <assert.h>

#include "two.h"
#include "event.h"
#include "http2_v3.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#include "logging.h"

#include "list_macros.h"

#ifndef TWO_MAX_CLIENTS
#define TWO_MAX_CLIENTS (EVENT_MAX_SOCKETS - 1)
#endif

// static variables
static event_loop_t loop;
static event_sock_t *server;
static void (*global_close_cb)();

void two_on_server_close(event_sock_t *server)
{
    (void)server;
    INFO("HTTP/2 server closed");
    if (global_close_cb != NULL) {
        global_close_cb();
    }
}

void two_on_client_close(event_sock_t *client) {
    INFO("HTTP/2 client (%d) closed.", client->descriptor);
}

void two_on_new_connection(event_sock_t *server, int status)
{
    (void)status;
    event_sock_t *client = event_sock_create(server->loop);
    if (event_accept(server, client) == 0) {
        INFO("New HTTP/2 client (%d) connected", client->descriptor);
        http2_new_client(client);
    }
    else {
        ERROR("The server cannot receive more clients (current maximum is %d). Increase CONFIG_EVENT_MAX_SOCKETS", EVENT_MAX_SOCKETS);
        event_close(client, two_on_client_close);
    }
}

int two_server_start(unsigned int port)
{
    event_loop_init(&loop);
    server = event_sock_create(&loop);

    int r = event_listen(server, port, two_on_new_connection);
    if (r < 0) {
        ERROR("Failed to open socket for listening");
        return 1;
    }
    INFO("Starting HTTP/2 server in port %u", port);
    event_loop(&loop);

    return 0;
}

void two_server_stop(void (*close_cb)())
{
    global_close_cb = close_cb;
    event_close(server, two_on_server_close);
}

int two_register_resource(char *method, char *path, http_resource_handler_t handler)
{
    return resource_handler_set(method, path, handler);
}
