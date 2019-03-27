#include <stdio.h>
#include <stdarg.h>

#include "logging.h"


void log_printf(level_t level, ...) {
    if (level < LEVEL) return;

    va_list args;
    va_start(args, level);
    switch(level) {
        case DEBUG:
            vfprintf(stderr, "[DEBUG] %s", args);
            break;
        case INFO:
            vfprintf(stderr, "[INFO] %s", args);
            break;
        case WARN:
            vfprintf(stderr, "[WARN] %s", args);
            break;
        case ERROR:
            vfprintf(stderr, "[ERROR] %s", args);
            break;
    }
    va_end(args);
}
