/**
 * Logging and debugging API
 *
 * Defines the DEBUG() INFO() WARN() and ERROR() macros that will print
 * a message depending on the level defined by the variable LOG_LEVEL (default is INFO)
 *
 * When debugging, it might be useful to set level to DEBUG, either by setting the constant
 * ENABLE_DEBUG, by using the -DENABLE_DEBUG on compilation, or by directly setting the LOG_LEVEL
 * variable to DEBUG, by using -DLOG_LEVEL=LOG_LEVEL_DEBUG
 *
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#ifndef LOGGING_H
#define LOGGING_H

#ifndef CONTIKI
#include <stdlib.h>
#endif
#include <string.h>
#include <stdio.h>
#include <errno.h>

// Log levels
#define LOG_LEVEL_DEBUG (1)
#define LOG_LEVEL_INFO  (2)
#define LOG_LEVEL_WARN  (3)
#define LOG_LEVEL_ERROR (4)
#define LOG_LEVEL_FATAL (5)
#define LOG_LEVEL_OFF   (6)

// Log modules
#define LOG_MODULE_SOCK    SOCK
#define LOG_MODULE_NET     NET
#define LOG_MODULE_HTTP2   HTTP2
#define LOG_MODULE_HTTP    HTTP
#define LOG_MODULE_FRAME   FRAME
#define LOG_MODULE_HPACK   HPACK

#ifndef LOG_LEVEL_SOCK
#define LOG_LEVEL_SOCK (LOG_LEVEL_OFF)
#endif

#ifndef LOG_LEVEL_NET
#define LOG_LEVEL_NET (LOG_LEVEL_OFF)
#endif

#ifndef LOG_LEVEL_HTTP2
#define LOG_LEVEL_HTTP2 (LOG_LEVEL_OFF)
#endif

#ifndef LOG_LEVEL_HTTP
#define LOG_LEVEL_HTTP (LOG_LEVEL_OFF)
#endif

#ifndef LOG_LEVEL_FRAME
#define LOG_LEVEL_FRAME (LOG_LEVEL_OFF)
#endif

#ifndef LOG_LEVEL_HPACK
#define LOG_LEVEL_HPACK (LOG_LEVEL_OFF)
#endif

// If LOG_MODULE is defined use LOG_LEVEL_<module>
// unless ENABLE_DEBUG is defined
// otherwise use LOG_LEVEL
#if defined(LOG_MODULE) && !defined(ENABLE_DEBUG)
#define __LOG_CONCAT0(x,y) x ## y
#define __LOG_CONCAT1(x,y)__LOG_CONCAT0(x,y)
#define __LOG_LEVEL(module) __LOG_CONCAT1(LOG_LEVEL_,module)
#else
#define __LOG_LEVEL(module) LOG_LEVEL
#endif

// Condition to check if logging is enabled
#define SHOULD_LOG(level, module) level >= __LOG_LEVEL(module)

#ifdef CONTIKI
#define LOG_EXIT(code) process_exit(PROCESS_CURRENT())
#define LOG_PRINT(...) printf(__VA_ARGS__)
#else
#define LOG_EXIT(code) exit(code)
#define LOG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#endif

#if defined(ENABLE_DEBUG)
#undef LOG_LEVEL
#define LOG_LEVEL (LOG_LEVEL_DEBUG)
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL (LOG_LEVEL_ERROR)
#endif

#if SHOULD_LOG(LOG_LEVEL_DEBUG, LOG_MODULE)
#define LOG(level, func, file, line, msg, ...) LOG_PRINT("[" #level "] " msg "; at %s:%d in %s()\n", ## __VA_ARGS__, file, line, func)
#else 
#define LOG(level, func, file, line, msg, ...) LOG_PRINT("[" #level "] " msg "\n", ## __VA_ARGS__)
#endif

// Macro to print debugging messages
#if SHOULD_LOG(LOG_LEVEL_DEBUG, LOG_MODULE)
#define DEBUG(...) LOG(DEBUG, __func__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

// Macro to print information messages
#if SHOULD_LOG(LOG_LEVEL_INFO, LOG_MODULE)
#define INFO(...) LOG(INFO, __func__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define INFO(...)
#endif

// Macro to print warning messages
#if SHOULD_LOG(LOG_LEVEL_WARN, LOG_MODULE)
#define WARN(...) LOG(WARN, __func__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define WARN(...)
#endif

// Macro to print error messages
#if SHOULD_LOG(LOG_LEVEL_ERROR, LOG_MODULE)
#define ERROR(msg, ...)                                                                                             \
    do {                                                                                                            \
        if (errno > 0) { LOG(ERROR, __func__, __FILE__, __LINE__, msg " (%s)", ## __VA_ARGS__, strerror(errno)); }  \
        else { LOG(ERROR, __func__, __FILE__, __LINE__, msg, ## __VA_ARGS__); }                                     \
    } while (0)
#else
#define ERROR(...)
#endif

// Macro to print fatal error messages
#if SHOULD_LOG(LOG_LEVEL_FATAL, LOG_MODULE)
#define FATAL(msg, ...)                                                                                             \
    do {                                                                                                            \
        if (errno > 0) { LOG(FATAL, __func__, __FILE__, __LINE__, msg " (%s)", ## __VA_ARGS__, strerror(errno)); }  \
        else { LOG(FATAL, __func__, __FILE__, __LINE__, msg, ## __VA_ARGS__); }                                     \
        LOG_EXIT(EXIT_FAILURE);                                                                                     \
    } while (0)
#else
#define FATAL(...)
#endif

// User messages
#ifndef DISABLE_PRINTF
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#endif /* LOGGING_H */
