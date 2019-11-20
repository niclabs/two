#ifndef CONFIG_H
#define CONFIG_H

#include "cbuf.h"

#define HTTP2_MAX_HBF_BUFFER 16384

// Logging per module
#undef LOG_LEVEL_NET
#define LOG_LEVEL_NET   (LOG_LEVEL_DEBUG)
#undef LOG_LEVEL_FRAME
#define LOG_LEVEL_FRAME (LOG_LEVEL_DEBUG)
#undef LOG_LEVEL_HTTP2
#define LOG_LEVEL_HTTP2 (LOG_LEVEL_DEBUG)

/*
* Signature of a callback, wrapped in a stuct
*
* buf_in:       circular buffer, net writes into it
* buf_out:      circular buffer, net reads from it
* state:        net allocates memory for this, but doesn't touch it. inits to 0
*/
typedef struct CALLBACK {
  struct CALLBACK (* func)(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
  void* debug_info; // just in case
} callback_t ;

#endif
