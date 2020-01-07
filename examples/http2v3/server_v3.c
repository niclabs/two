#include <stdlib.h>
#include <signal.h>

#include "event.h"
#include "http2_v3.h"

#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "logging.h"


static event_loop_t loop;
static event_sock_t *server;

void on_server_close(event_sock_t * server) {
    (void)server;
    INFO("Server closed");
    exit(0);
}

void on_client_close(event_sock_t *client) {
    (void)client;
}

void on_new_client(event_sock_t *server, int status)
{

    if (status < 0) {
        ERROR("Cannot receive more clients");
        return;
    }
    
    event_sock_t *client = event_sock_create(server->loop);
    if (event_accept(server, client) == 0) {
        INFO("New client connection");

        // http2 new client
        http2_new_client(client);
    }
    else {
        event_close(client, on_client_close);
    }
}

void start()
{
    event_loop_init(&loop);
    server = event_sock_create(&loop);

    event_listen(server, 8888, on_new_client);
    event_loop(&loop);
}

void cleanup(int sig) {
    (void)sig;
    event_close(server, on_server_close);
}

int main()
{
    // TODO: register resource
    // set cleanup signal
    signal(SIGINT, cleanup);
    // start server
    start();
}
