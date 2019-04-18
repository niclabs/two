/**
 * Logging and debugging API
 * 
 * Defines the DEBUG() INFO() WARN() and ERROR() macros that will print
 * a message depending on the level defined by the variable LEVEL (default is INFO)
 * 
 * When debugging, it might be useful to set level to DEBUG, either by setting the variable
 * ENABLE_DEBUG to 1, by using the -DENABLE_DEBUG=1 on compilation, or by directly setting the LEVEL
 * variable to DEBUG, by using -DLEVEL=DEBUG
 * 
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

typedef enum
{
    DEBUG,
    INFO,
    WARN,
    ERROR
} level_t;

#define LOGGING_MAX_MSG_LEN (256)

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG (0)
#elif (ENABLE_DEBUG != 0)
#define LEVEL (DEBUG)
#endif

#ifndef LEVEL
#define LEVEL (INFO)
#endif

#ifndef LOGGING_EXIT_ON_ERROR
#define LOGGING_EXIT_ON_ERROR (0)
#endif

#define LOG(level, func, file, line, fmt, ...)                                                                   \
    do                                                                                                           \
    {                                                                                                            \
        if (level >= LEVEL)                                                                                      \
        {                                                                                                        \
            char msg[256];                                                                                       \
            snprintf(msg, LOGGING_MAX_MSG_LEN, fmt, ##__VA_ARGS__);                                              \
            switch (level)                                                                                       \
            {                                                                                                    \
            case DEBUG:                                                                                          \
                fprintf(stderr, "[DEBUG] %s: %s() in %s:%d\n", msg, func, file, line);                           \
                break;                                                                                           \
            case INFO:                                                                                           \
                fprintf(stderr, "[INFO] %s: %s() in %s:%d\n", msg, func, file, line);                            \
                break;                                                                                           \
            case WARN:                                                                                           \
                fprintf(stderr, "[WARN] %s: %s() in %s:%d\n", msg, func, file, line);                            \
                break;                                                                                           \
            case ERROR:                                                                                          \
                if (errno > 0)                                                                                   \
                    fprintf(stderr, "[ERROR] %s (%s): %s() in %s:%d\n", msg, strerror(errno), func, file, line); \
                else                                                                                             \
                    fprintf(stderr, "[ERROR] %s: %s() in %s:%d\n", msg, func, file, line);                       \
                if (LOGGING_EXIT_ON_ERROR)                                                                       \
                    exit(EXIT_FAILURE);                                                                          \
                break;                                                                                           \
            }                                                                                                    \
        }                                                                                                        \
    } while (0);

// Macro to print debugging messages
#define DEBUG(...) LOG(DEBUG, __func__, __FILE__, __LINE__, __VA_ARGS__)

// Macro to print information messages
#define INFO(...) LOG(INFO, __func__, __FILE__, __LINE__, __VA_ARGS__)

// Macro to print warning messages
#define WARN(...) LOG(WARN, __func__, __FILE__, __LINE__, __VA_ARGS__)

// Macro to print error messages
#define ERROR(...) LOG(ERROR, __func__, __FILE__, __LINE__, __VA_ARGS__)

#endif /* LOGGING_H */
