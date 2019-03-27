/**
 * Basic tcp server using select()
 * Based on https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
 *
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#include <signal.h>

#include "logging.h"
#include "server.h"

#define PORT (8888)

server_t * server;

void cleanup(int signal) {
    (void)signal;

    INFO("Ctrl-C received. Terminating server.\n");
}

int main(void)
{
    /* Create the socket and set it up to accept connections. */
    server = server_create(PORT);

    // Trap Ctrl-C
    signal(SIGINT, cleanup);

    // Start listening for connections
    server_listen(server);

    // Cleanup after listen terminates
    server_destroy(server);
}
