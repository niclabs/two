#include <string.h>
#include <stdio.h>

#if !defined(CONTIKI) || defined(CONTIKI_TARGET_NATIVE)
#include <stdlib.h>
#include <signal.h>
#endif

#ifdef CONTIKI
#include "contiki.h"
#endif

#include "two.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#include "logging.h"

#ifdef CONTIKI
PROCESS(example_server_process, "http/2 example server");
AUTOSTART_PROCESSES(&example_server_process);
#endif

void on_server_close()
{
#if !defined(CONTIKI) || defined(CONTIKI_TARGET_NATIVE)
    exit(0);
#endif
}

void cleanup(int sig)
{
    (void)sig;
    DEBUG("Ctrl-C received, closing server");
    two_server_stop(on_server_close);
}

int send_text(char *method, char *uri, uint8_t *response, int maxlen)
{
    (void)method;
    (void)uri;
    if (maxlen < 16) {
        memcpy(response, "", 0);
        return 0;
    }
    memcpy(response, "hello world!!!!", 16);
    return 16;
}


#ifdef CONTIKI
PROCESS_THREAD(example_server_process, ev, data)
#else
int main(int argc, char **argv)
#endif
{
#ifdef CONTIKI
    PROCESS_BEGIN();

    int port = 8888;
#else
    if (argc < 2) {
        ERROR("Usage: %s <port>", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port < 0) {
        ERROR("Invalid port given");
        return 1;
    }
#endif

#if !defined(CONTIKI) || defined(CONTIKI_TARGET_NATIVE)
    signal(SIGINT, cleanup);
#endif

    // Register resource
    if (two_register_resource("GET", "/index", &send_text) < 0) {
        FATAL("Failed to register resource /index");
    }

    if (two_server_start(port) < 0) {
        FATAL("Failed to start server");
    }

#ifdef CONTIKI
    PROCESS_END();
#endif
}
