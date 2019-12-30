#ifndef CONFIG_H
#define CONFIG_H

#include "cbuf.h"

// HPACK macros
#define CONFIG_INCLUDE_DYNAMIC_TABLE 1
#define CONFIG_MAX_TABLE_SIZE 256

// HTTP2 macros
#define CONFIG_MAX_HBF_BUFFER 256
#define CONFIG_MAX_BUFFER_SIZE 256

// Logging per module
#define ENABLE_LOG 0
#if ENABLE_LOG
    #undef LOG_LEVEL_EVENT
    #define LOG_LEVEL_EVENT (LOG_LEVEL_DEBUG)
    #undef LOG_LEVEL_FRAME
    #define LOG_LEVEL_FRAME (LOG_LEVEL_DEBUG)
    #undef LOG_LEVEL_HTTP2
    #define LOG_LEVEL_HTTP2 (LOG_LEVEL_DEBUG)
    #undef LOG_LEVEL_HPACK
    #define LOG_LEVEL_HPACK (LOG_LEVEL_DEBUG)
#endif

#endif
