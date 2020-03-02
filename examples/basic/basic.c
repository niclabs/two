#include <signal.h>
#include <stdlib.h>

#include "logging.h"
#include "two.h"

void on_server_close()
{
    exit(0);
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

int main(int argc, char **argv)
{
    if (argc < 2) {
        ERROR("Usage: %s <port>", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port < 0) {
        ERROR("Invalid port given");
        return 1;
    }

    signal(SIGINT, cleanup);

    // Register resource
    two_register_resource("GET", "/", "text/plain", hello_world);
    if (two_server_start(port) < 0) {
        ERROR("Failed to start server");
    }
}
