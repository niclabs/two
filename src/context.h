/**
 * Defines HTTP/2 client context operations
 * 
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#ifndef CONTEXT_H
#define CONTEXT_H

/**
 * Client connection context (status, fd, etc.)
 * to be used either for server or client side implementation
 */
typedef struct client_ctx {
    int fd;

    // TODO: other http2 parameters go here
} client_ctx_t;

void client_ctx_init(int fd, client_ctx_t * client);
void client_ctx_destroy(client_ctx_t * client);

#endif /* CONTEXT_H */