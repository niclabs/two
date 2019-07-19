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

#include <stdlib.h>
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

#ifdef WITH_CONTIKI
#define LOG_EXIT(code) process_exit(PROCESS_CURRENT())
#define LOG_PRINT(...) printf(__VA_ARGS__)
#else
#define LOG_EXIT(code) exit(code)
#define LOG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#endif

#if defined(ENABLE_DEBUG)
#define LOG_LEVEL (LOG_LEVEL_DEBUG)
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL (LOG_LEVEL_ERROR)
#endif

#if LOG_LEVEL_DEBUG >= LOG_LEVEL
#define LOG(level, func, file, line, msg, ...) LOG_PRINT("[" #level "] " msg "; at %s:%d in %s()\n", ## __VA_ARGS__, file, line, func)
#else 
#define LOG(level, func, file, line, msg, ...) LOG_PRINT("[" #level "] " msg "\n", ## __VA_ARGS__)
#endif

// Macro to print debugging messages
#if LOG_LEVEL_DEBUG >= LOG_LEVEL
#define DEBUG(...) LOG(DEBUG, __func__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

// Macro to print information messages
#if LOG_LEVEL_INFO >= LOG_LEVEL
#define INFO(...) LOG(INFO, __func__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define INFO(...)
#endif

// Macro to print warning messages
#if LOG_LEVEL_WARN >= LOG_LEVEL
#define WARN(...) LOG(WARN, __func__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define WARN(...)
#endif

// Macro to print error messages
#if LOG_LEVEL_ERROR >= LOG_LEVEL
#define ERROR(msg, ...)                                                                                             \
    do {                                                                                                            \
        if (errno > 0) { LOG(ERROR, __func__, __FILE__, __LINE__, msg " (%s)", ## __VA_ARGS__, strerror(errno)); }  \
        else { LOG(ERROR, __func__, __FILE__, __LINE__, msg, ## __VA_ARGS__); }                                     \
    } while (0)
#else
#define ERROR(...)
#endif

// Macro to print fatal error messages
#if LOG_LEVEL_FATAL >= LOG_LEVEL
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
