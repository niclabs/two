#include <string.h>

#include "cbuf.h"
#include "logging.h"

#undef MIN
#define MIN(n, m)   (((n) < (m)) ? (n) : (m))

void cbuf_init(cbuf_t *cbuf, void *buf, int maxlen)
{
    cbuf->ptr = buf;
    cbuf->head = 0;
    cbuf->tail = 0;
    cbuf->maxlen = maxlen;
    cbuf->len = 0;
    cbuf->state = CBUF_OPEN;
}

int cbuf_push(cbuf_t *cbuf, void *src, int len)
{
    int bytes = 0;
    while (len > 0 && cbuf->len < cbuf->maxlen && cbuf->state == CBUF_OPEN) {
        int copylen = MIN(len, cbuf->maxlen - cbuf->head);
        if (cbuf->head < cbuf->tail) {
            copylen = MIN(len, cbuf->tail - cbuf->head);
        }
        memcpy(cbuf->ptr + cbuf->head, src, copylen);

        // Update write index
        cbuf->head = (cbuf->head + copylen) % cbuf->maxlen;

        // Update used count
        cbuf->len += copylen;

        // Update result
        bytes += copylen;

        // Update pointers
        src += copylen;
        len -= copylen;
    }

    return bytes;
}

int cbuf_pop(cbuf_t *cbuf, void *dst, int len)
{
    int bytes = 0;

    while (len > 0 && cbuf->len > 0) {
        int copylen = MIN(len, cbuf->maxlen - cbuf->tail);
        if (cbuf->tail < cbuf->head) {
            copylen = MIN(len, cbuf->head - cbuf->tail);
        }
       
        // if dst is NULL, calls to read will only increase the read pointer
        if (dst != NULL) { 
            memcpy(dst, cbuf->ptr + cbuf->tail, copylen);
            dst += copylen;
        }

        // Update read pointer
        cbuf->tail = (cbuf->tail + copylen) % cbuf->maxlen; 

        // Update used count
        cbuf->len -= copylen;

        // update result
        bytes += copylen;

        // Update pointers
        len -= copylen;
    }

    return bytes;
}

int cbuf_peek(cbuf_t *cbuf, void *dst, int len)
{
    int bytes = 0;
    int cbuflen = cbuf->len;
    int tail = cbuf->tail;

    while (len > 0 && cbuflen > 0) {
        int copylen = MIN(len, cbuf->maxlen - tail);
        if (tail < cbuf->head) {
            copylen = MIN(len, cbuf->head - tail);
        }

        if (dst != NULL) {
            memcpy(dst, cbuf->ptr + tail, copylen);
            dst += copylen;
        }

        // Update read pointer
        tail = (tail + copylen) % cbuf->maxlen; 

        // Update used count
        cbuflen -= copylen;

        // update result
        bytes += copylen;

        // Update pointers
        len -= copylen;
    }

    return bytes;
}


int cbuf_len(cbuf_t * cbuf) {
    return cbuf->len;
}

int cbuf_maxlen(cbuf_t * cbuf) {
    return cbuf->maxlen;
}

void cbuf_end(cbuf_t * cbuf) {
    cbuf->state = CBUF_ENDED;
}

int cbuf_has_ended(cbuf_t * cbuf) {
    return cbuf->state == CBUF_ENDED;
}
