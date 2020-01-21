#ifndef CONFIG_H
#define CONFIG_H

// HTTP2 macros
#define CONFIG_MAX_HBF_BUFFER 256
#define CONFIG_MAX_BUFFER_SIZE 256
/*Local server settings values*/
#define CONFIG_HEADER_TABLE_SIZE 256
#define CONFIG_ENABLE_PUSH 0
#define CONFIG_MAX_CONCURRENT_STREAMS 1
#define CONFIG_INITIAL_WINDOW_SIZE 4096
#define CONFIG_MAX_FRAME_SIZE 16384
#define CONFIG_MAX_HEADER_LIST_SIZE 512

// HPACK macros
#define CONFIG_INCLUDE_DYNAMIC_TABLE 1
#if CONFIG_INCLUDE_DYNAMIC_TABLE == 0
#undef CONFIG_HEADER_TABLE_SIZE
#define CONFIG_HEADER_TABLE_SIZE 0
#endif
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
