#ifndef VOXELS_UTIL_H
#define VOXELS_UTIL_H

#include <stdarg.h>
#include <iostream>

void Log(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}

#endif
