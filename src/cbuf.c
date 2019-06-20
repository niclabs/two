#include <string.h>

#include "cbuf.h"
#include "logging.h"

#ifndef MIN
#define MIN(n, m)   (((n) < (m)) ? (n) : (m))
#endif

void cbuf_init(cbuf_t *cbuf, void *buf, int len)
{
    cbuf->ptr = buf;
    cbuf->readptr = buf;
    cbuf->writeptr = buf;
    cbuf->size = len;
    cbuf->count = 0;
}

int cbuf_write(cbuf_t *cbuf, void *src, int len)
{
    int bytes = 0;
    while (len > 0 && cbuf->count < cbuf->size) {
        int copylen = MIN(len, cbuf->size - (cbuf->writeptr - cbuf->ptr));
        if (cbuf->writeptr < cbuf->readptr) {
            copylen = MIN(len, cbuf->readptr - cbuf->writeptr);
        }
        memcpy(cbuf->writeptr, src, copylen);

        // Update write pointer
        cbuf->writeptr = cbuf->ptr + (cbuf->writeptr - cbuf->ptr + copylen) % cbuf->size;

        // Update used count
        cbuf->count += copylen;

        // Update result
        bytes += copylen;

        // Update pointers
        src += copylen;
        len -= copylen;
    }

    return bytes;
}

int cbuf_read(cbuf_t *cbuf, void *dst, int len)
{
    int bytes = 0;

    while (len > 0 && cbuf->count > 0) {
        int copylen = MIN(len, cbuf->size - (cbuf->readptr - cbuf->ptr));
        if (cbuf->readptr < cbuf->writeptr) {
            copylen = MIN(len, cbuf->writeptr - cbuf->readptr);
        }
        memcpy(dst, cbuf->readptr, copylen);

        // Update read pointer
        cbuf->readptr = cbuf->ptr + (cbuf->readptr - cbuf->ptr + copylen) % cbuf->size;

        // Update used count
        cbuf->count -= copylen;

        // update result
        bytes += copylen;

        // Update pointers
        dst += copylen;
        len -= copylen;
    }

    return bytes;
}
