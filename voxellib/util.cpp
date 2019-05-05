#include <cstdarg>
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

void log_mat4(const char *pre, const glm::mat4 &mat) {
    log("%s===", pre);
    for (int i = 0; i < 4; ++i) {
        log("(%f, %f, %f, %f)", mat[i][0], mat[i][1], mat[i][2], mat[i][3]);
    }
    log("===");
}

