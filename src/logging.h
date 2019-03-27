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

/**
 * Prints message to stderr only if the defined level 
 * is equal or above the level defined by the variable 
 * LEVEL
 */
void log_printf(level_t level, ...);

// Macro to print debugging messages
#define DEBUG(...) log_printf(DEBUG, __VA_ARGS__)

// Macro to print information messages
#define INFO(...) log_printf(INFO, __VA_ARGS__)

// Macro to print warning messages
#define WARN(...) log_printf(WARN, __VA_ARGS__)

// Macro to print error messages
#define ERROR(...) log_printf(ERROR, __VA_ARGS__)


#endif
