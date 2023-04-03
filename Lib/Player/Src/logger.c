#include "logger.h"
#include <stdarg.h>
#include <stdio.h>

void log_message(LogLevel level, const char *format, ...) {
    va_list args;
    va_start(args, format);

    vprintf(format, args);

    va_end(args);
}
