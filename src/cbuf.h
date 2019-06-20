#ifndef CBUF_H
#define CBUF_H

/**
 * Circular buffer definition functions
 *
 * A circular buffer allows to read and write from a buffer
 * using all allocated memory. Write operations reduce 
 * the available space and read operations increase
 * the available buffer. This obviously also means that 
 * data can only be read once from the buffer using the cbuf_read()
 * call.
 **/


typedef struct {
    void * ptr;
    int size;
    int count;

    // read and write pointer
    void * readptr;
    void * writeptr;
} cbuf_t;


/** 
 * Initialize circular buffer with the specified
 * memory pointer and length
 */
void cbuf_init(cbuf_t * cbuf, void * buf, int len);

/**
 * Write data to the circular buffer
 */
int cbuf_write(cbuf_t * cbuf, void * src, int len);

/**
 * Read data from the circular buffer
 */
int cbuf_read(cbuf_t * cbuf, void * dst, int len);

/**
 * Return available read size
 */
int cbuf_size(cbuf_t * cbuf);
#endif /* CBUF_H */
