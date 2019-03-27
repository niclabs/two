#ifndef LOG_H
#define LOG_H

typedef enum {
    DEBUG,
    INFO,
    WARN,
    ERROR
} level_t;

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG (0)
#elif (ENABLE_DEBUG != 0)
#define LEVEL (DEBUG)
#endif

#ifndef LEVEL
#define LEVEL (INFO)
#endif

void log_printf(level_t level, ...);


#define DEBUG(...) log_printf(DEBUG, __VA_ARGS__)
#define INFO(...) log_printf(INFO, __VA_ARGS__)
#define WARN(...) log_printf(WARN, __VA_ARGS__)
#define ERROR(...) log_printf(ERROR, __VA_ARGS__)


#endif
