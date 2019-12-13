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
#undef LOG_LEVEL_HPACK
#define LOG_LEVEL_HPACK (LOG_LEVEL_DEBUG)
 

#endif
