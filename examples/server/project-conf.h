#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* QUEUEBUF_CONF_NUM specifies the number of queue buffers. Queue
   buffers are used throughout the Contiki netstack but the
   configuration option can be tweaked to save memory. Performance can
   suffer with a too low number of queue buffers though.

   We reduce the queue buffer size to allow the two implementation to fit
   in the .bss memory section. */
#define QUEUEBUF_CONF_NUM 4

#endif
