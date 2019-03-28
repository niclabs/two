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
typedef struct context {
    int fd;

    // TODO: other http2 parameters go here
} context_t;

void context_init(int fd, context_t * ctx);
void context_destroy(context_t * ctx);

#endif /* CONTEXT_H */