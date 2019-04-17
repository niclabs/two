/**
 * Implementation of logging API log_printf() function
 * 
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "logging.h"


void log_printf(level_t level, const char * func, const char * file, int line, ...) {
    if (level < LEVEL) return;

    char msg[256];
    va_list args;
    va_start(args, line);

    // Get format from args
    char * fmt = va_arg(args, char *);

    // Print message to string
    vsnprintf(msg, LOGGING_MAX_MSG_LEN, fmt, args);

    // close va list
    va_end(args);
    
    switch(level) {
        case DEBUG:
            fprintf(stderr, "[DEBUG] %s: %s() in %s:%d\n", msg, func, file, line);
            break;
        case INFO:
            fprintf(stderr, "[INFO] %s: %s() in %s:%d\n", msg, func, file, line);
            break;
        case WARN:
            fprintf(stderr, "[WARN] %s: %s() in %s:%d\n", msg, func, file, line);
            break;
        case ERROR:
            if (errno > 0) {
                fprintf(stderr, "[ERROR] %s (%s): %s() in %s:%d\n", msg, strerror(errno), func, file, line);
            }
            else {
                fprintf(stderr, "[ERROR] %s: %s() in %s:%d\n", msg, func, file, line);
            }
            if (LOGGING_EXIT_ON_ERROR) exit(EXIT_FAILURE);
            break;
    }
}
