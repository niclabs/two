#ifdef CONTIKI_TARGET_NATIVE
#include <signal.h>
#include <stdlib.h>
#endif

#include "contiki.h"
#include "logging.h"
#include "two.h"

PROCESS(example_server_process, "http/2 example server");
AUTOSTART_PROCESSES(&example_server_process);

void on_server_close()
{
#ifdef CONTIKI_TARGET_NATIVE
    exit(0);
#endif
}

void cleanup(int sig)
{
    (void)sig;
    PRINTF("Ctrl-C received, closing server\n");
    two_server_stop(on_server_close);
}

int hello_world(char *method, char *uri, char *response, unsigned int maxlen)
{
    (void)method;
    (void)uri;

    return snprintf(response, maxlen, "Hello, World!!!\n");
}

PROCESS_THREAD(example_server_process, ev, data)
{
    PROCESS_BEGIN();

    int port = 8888;

#ifdef CONTIKI_TARGET_NATIVE
    signal(SIGINT, cleanup);
#endif

    // Register resource
    two_register_resource("GET", "/", "text/plain", hello_world);
    if (two_server_start(port) < 0) {
        ERROR("Failed to start server");
    }

    PROCESS_END();
}
