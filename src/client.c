/**
 * Client API implementation
 *
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/time.h>

#include "constants.h"
#include "logging.h"
#include "client.h"
#include "context.h"

struct client {
    enum client_state {
        IDLE,
        OPEN,
        CONNECTED
    } state;
    struct sockaddr_in6 dst;
    context_t ctx;
};

client_t client_g;

/**
 * handle_event callback for client handler
 */
static void client_wait_receive(void *instance, int secs)
{
    client_t *client = instance;
    int fd = client->ctx.fd;

    struct timeval timeout;
    timeout.tv_sec = secs;
    timeout.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                   sizeof(timeout)) < 0) {
        ERROR("Setting timeout: %s\n", strerror(errno));
    }

    char buffer[MAX_BUF_SIZE];
    int nbytes;

    nbytes = read(fd, buffer, MAX_BUF_SIZE);
    if (nbytes < 0) {
        if (errno == EAGAIN) {
            // Received timeout
            WARN("Read timeout. Terminating connection ...\n");
            client_destroy(client);
            return;
        }

        /* Read error. */
        ERROR("In read(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    else if (nbytes == 0) {
        client_destroy(client);
        return;
    }
    else {
        /* Data read. */
        INFO("Received message: '%.*s'\n", nbytes - 1, buffer);
    }

    // Unset timeout
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                   sizeof(timeout)) < 0) {
        ERROR("Unsetting timeout: %s\n", strerror(errno));
    }
}

static void on_client_connect(client_t *client)
{
    int fd = client->ctx.fd;

    if (write(fd, PREFACE, strlen(PREFACE)) < 0) {
        ERROR("Error in sending preface: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

client_t *client_create(char *addr, uint16_t port)
{
    assert(client_g.state == IDLE);

    int sock;
    struct sockaddr_in6 dst;

    // parse destination address
    if (inet_pton(AF_INET6, addr, &dst.sin6_addr) != 1) {
        ERROR("Unable to parse destination address: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    dst.sin6_family = AF_INET6;
    dst.sin6_port = htons(port);

    if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        ERROR("In socket(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    client_t *c = &client_g;
    c->ctx.fd = sock;
    c->dst = dst;
    c->state = OPEN;

    return c;
}

void client_connect(client_t *client)
{
    assert(client != NULL);
    assert(client->state == OPEN);

    if (connect(client->ctx.fd, (struct sockaddr *)&client->dst, sizeof(client->dst)) < 0) {
        ERROR("Error in connect %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    client->state = CONNECTED;

    // Call client
    on_client_connect(client);
}

void client_request(client_t *client, char *endpoint)
{
    assert(client != NULL);
    assert(client->state == CONNECTED);

    int fd = client->ctx.fd;
    if (write(fd, endpoint, strlen(endpoint)) < 0) {
        ERROR("Error in sending request: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Wait 5 seconds to receive message
    client_wait_receive(client, 5);
}

void client_destroy(client_t *client) {
    assert(client != NULL);
    
    int fd = client->ctx.fd;
    close(fd);
    client->state = IDLE;
}
