#ifndef CBUF_H
#define CBUF_H

/**
 * Circular buffer definition functions
 *
 * A circular buffer allows to read and write from a buffer
 * using all allocated memory. Write operations reduce
 * the free size and read operations increase the size by
 * maintaining a read and write pointer
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
#endif /* CBUF_H */
