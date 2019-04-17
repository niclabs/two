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

/**
 * Prints message to stderr only if the defined level 
 * is equal or above the level defined by the variable 
 * LEVEL
 */
void log_printf(level_t level, const char * func, const  char * file, int line, ...);

// Macro to print debugging messages
#define DEBUG(...) log_printf(DEBUG, __func__, __FILE__, __LINE__, __VA_ARGS__)

// Macro to print information messages
#define INFO(...) log_printf(INFO, __func__, __FILE__, __LINE__, __VA_ARGS__)

// Macro to print warning messages
#define WARN(...) log_printf(WARN, __func__, __FILE__, __LINE__, __VA_ARGS__)

// Macro to print error messages
#define ERROR(...) log_printf(ERROR, __func__, __FILE__, __LINE__, __VA_ARGS__)


#endif
