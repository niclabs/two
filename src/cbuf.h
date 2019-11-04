#ifndef CBUF_H
#define CBUF_H

/**
 * Circular buffer definition functions
 *
 * A circular buffer is a fixed size FIFO structure 
 * for arbitrary data types. It is called circular because
 * it treats the available memory as as a circle (read/write operations
 * go back to the beginning after reaching the end of the buffer)
 *
 * Push operations append to the end of the buffer, 
 * and reduce the available buffer size. 
 *
 * Pop operations retrieve data from the beginning of the 
 * buffer and increase the available buffer size
 **/


typedef struct {
    void * ptr;
    int maxlen;
    int len;

    // read and write pointer
    void * readptr;
    void * writeptr;
} cbuf_t;


/** 
 * Initialize circular buffer with the specified
 * memory pointer and length
 */
void cbuf_init(cbuf_t * cbuf, void * buf, int maxlen);

/**
 * Push data to the end of the circular buffer
 */
int cbuf_push(cbuf_t * cbuf, void * src, int len);

/**
 * Pop data from the beginning of the circular buffer
 */
int cbuf_pop(cbuf_t * cbuf, void * dst, int len);

/**
 * Peek data from the circular buffer without altering
 * the read pointer
 */
int cbuf_peek(cbuf_t * cbuf, void * dst, int len);

/**
 * Return available read size
 */
int cbuf_len(cbuf_t * cbuf);

/**
 * Return maximum read size
 */
int cbuf_maxlen(cbuf_t * cbuf);
#endif /* CBUF_H */
