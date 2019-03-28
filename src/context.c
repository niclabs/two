#include <assert.h>
#include <unistd.h>

#include "context.h"

void context_init(int fd, context_t * ctx) {
    assert(ctx != NULL);

    ctx->fd = fd;
    // TODO: perform http2 state initialization
}

void context_destroy(context_t * ctx) {
    assert(ctx != NULL);

    // TODO: perform closeup tasks
    close(ctx->fd);
}