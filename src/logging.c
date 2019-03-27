/**
 * Implementation of logging API log_printf() function
 * 
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#include <stdio.h>
#include <stdarg.h>

#include "logging.h"


void log_printf(level_t level, ...) {
    if (level < LEVEL) return;

    va_list args;
    va_start(args, level);

    char * fmt = va_arg(args, char *);
    switch(level) {
        case DEBUG:
            fprintf(stderr, "[DEBUG] ");
            break;
        case INFO:
            fprintf(stderr, "[INFO] ");
            break;
        case WARN:
            fprintf(stderr, "[WARN] ");
            break;
        case ERROR:
            fprintf(stderr, "[ERROR] ");
            break;
    }

    vfprintf(stderr, fmt, args);
    va_end(args);
}
