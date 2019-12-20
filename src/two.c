/*
   This API contains the "dos" methods to be used by
   HTTP/2
 */

#include <assert.h>

#include "two.h"
#include "event.h"
#include "http2/http2.h"
#include "http2/structs.h"
#include "logging.h"

#ifndef TWO_MAX_CLIENTS
#define TWO_MAX_CLIENTS (EVENT_MAX_SOCKETS - 1)
#endif

// static variables
static event_loop_t loop;
static event_sock_t *server;
static two_close_cb global_close_cb;

// HTTP/2 client memory
static h2states_t http2_state_list[TWO_MAX_CLIENTS];
static h2states_t *connected;
static h2states_t *unused;

// methods
h2states_t *get_unused_state()
{
    if (unused == NULL) {
        return NULL;
    }

    h2states_t *ctx = unused;
    unused = ctx->next;

    ctx->next = connected;
    connected = ctx;

    // reset memory
    memset(ctx, 0, sizeof(h2states_t));
    return ctx;
}

void two_free_client(h2states_t *ctx)
{
    h2states_t *curr = connected, *prev = NULL;

    while (curr != NULL) {
        if (curr == ctx) {
            if (prev == NULL) {
                connected = curr->next;
            }
            else {
                prev->next = curr->next;
            }

            curr->next = unused;
            unused = curr;

            return;
        }

        prev = curr;
        curr = curr->next;
    }

}

void two_on_server_close(event_sock_t *server)
{
    (void)server;
    INFO("Server closed");
    if (global_close_cb != NULL) {
        global_close_cb();
    }
}

void two_on_client_close(event_sock_t *client)
{
    INFO("Client connection closed");
    two_free_client(client->data);
}

void two_on_new_connection(event_sock_t *server, int status)
{
    if (status < 0) {
        ERROR("Cannot receive more clients");
        return;
    }

    event_sock_t *client = event_sock_create(server->loop);
    if (event_accept(server, client) == 0) {
        h2states_t *data = get_unused_state();

        // the number of h2state should be equal to the number of clients
        // if this happens there is an implementation error
        assert(data != NULL);

        INFO("New client connection");

        // Configure data
        client->data = data;

        // Initialize http2 client
        http2_server_init_connection(client, status);
    }
    else {
        ERROR("No more clients available");
        event_close(client, two_on_client_close);
    }
}

int two_server_start(unsigned int port)
{
    // Initialize client memory
    memset(http2_state_list, 0, TWO_MAX_CLIENTS * sizeof(h2states_t));
    for (int i = 0; i < TWO_MAX_CLIENTS - 1; i++) {
        http2_state_list[i].next = &http2_state_list[i + 1];
    }
    connected = NULL;
    unused = &http2_state_list[0];

    event_loop_init(&loop);
    server = event_sock_create(&loop);

    int r = event_listen(server, port, two_on_new_connection);

    INFO("Starting http/2 server in port 8888");
    if (r < 0) {
        ERROR("Could not start server");
        return 1;
    }
    event_loop(&loop);

    return 0;
}

void two_server_stop(two_close_cb close_cb)
{
    global_close_cb = close_cb;
    event_close(server, two_on_server_close);
}

int two_register_resource(char *method, char *path, http_resource_handler_t handler)
{
    return resource_handler_set(method, path, handler);
}
