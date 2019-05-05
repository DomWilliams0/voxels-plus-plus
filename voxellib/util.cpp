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

void resolve_resource_path(std::string &out, const char *relative_path) {
    out.clear();

    char *env_value = std::getenv("VOXELS_PATH");
    if (env_value) {
        out.append(env_value);
        out.append("/");
    }

    out.append("voxellib/res/");
    out.append(relative_path);
}

