#include <assert.h>
#include <unistd.h>

#include "context.h"

void client_ctx_init(int fd, client_ctx_t * ctx) {
    assert(ctx != NULL);

    ctx->fd = fd;

    // TODO: perform http2 state initialization
}

void client_ctx_destroy(client_ctx_t * ctx) {
    assert(ctx != NULL);

    // TODO: perform closeup tasks
    close(ctx->fd);
}