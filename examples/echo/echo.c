#include <errno.h>

#if !defined(CONTIKI) || defined(CONTIKI_TARGET_NATIVE)
#include <stdlib.h>
#include <signal.h>
#endif

#ifdef CONTIKI
#include "contiki.h"
#endif

#include "event.h"

#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "logging.h"
 
event_loop_t loop;
event_sock_t *server;
uint8_t event_buf[1024];

#ifdef CONTIKI
PROCESS(echo_server_process, "Echo server process");
AUTOSTART_PROCESSES(&echo_server_process);
#endif

void on_server_close(event_sock_t *handle) {
    (void)handle;
    INFO("Server closed");
#if !defined(CONTIKI) || defined(CONTIKI_TARGET_NATIVE)
    exit(0);
#elif defined(CONTIKI)
    process_exit(&echo_server_process);
#endif
}

void close_server(int sig) {
    (void)sig;
    DEBUG("Signal caught, closing server");
    event_close(server, on_server_close);
}

void on_client_close(event_sock_t *handle) {
    (void)handle;
    INFO("Client closed");
}

void echo_write(event_sock_t *client, int status) {
    (void)client;
    if (status < 0) {
        ERROR("Failed to write");
    }
}

int echo_read(event_sock_t *client, int nread, uint8_t *buf)
{
    if (nread > 0) {
        DEBUG("Read '%.*s'", (int)nread - 1, buf);
        event_write(client, nread, buf, echo_write);
        
        return nread;
    }

    INFO("Remote connection closed");
    event_close(client, on_client_close);

    return 0;
}

int echo_timeout(event_sock_t *client) {
    (void)client;
    DEBUG("No read activity in 5s");
    return 0;
}

void on_new_connection(event_sock_t *server, int status)
{
    if (status < 0) {
        ERROR("Connection error");
        // error!
        return;
    }

    INFO("New client connection");
    event_sock_t *client = event_sock_create(server->loop);
    if (event_accept(server, client) == 0) {
        event_read_timeout(client, 5000, echo_timeout);
        event_read_start(client, event_buf, 1024, echo_read);
    }
    else {
        event_close(client, on_client_close);
    }
}

#ifdef CONTIKI
PROCESS_THREAD(echo_server_process, ev, data)
#else
int main()
#endif
{
#ifdef CONTIKI
    PROCESS_BEGIN();
#endif
    event_loop_init(&loop);
    server = event_sock_create(&loop);

#if !defined(CONTIKI) || defined(CONTIKI_TARGET_NATIVE)
    signal(SIGINT, close_server);
#endif

    int r = event_listen(server, 8888, on_new_connection);
    if (r < 0) {
        ERROR("Could not start server");
        return 1;
    }
    event_loop(&loop);

#ifdef CONTIKI
    PROCESS_END();
#endif
}
