#include <stdarg.h>
#include <iostream>
#include "util.h"

void log(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    fputc('\n', stdout);
    va_end(ap);
}

