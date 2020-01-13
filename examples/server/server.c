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
    INFO("Ctrl-C received, closing server");
    two_server_stop(on_server_close);
}

int hello_world(char *method, char *uri, uint8_t *response, int maxlen)
{
    (void)method;
    (void)uri;
    memcpy(response, "hello world!!!\n", maxlen);
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
    two_register_resource("GET", "/", &hello_world);
    if (two_server_start(port) < 0) {
        ERROR("Failed to start server");
    }

#ifdef CONTIKI
    PROCESS_END();
#endif
}
