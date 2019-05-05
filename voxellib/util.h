#ifndef VOXELS_UTIL_H
#define VOXELS_UTIL_H

#include <string>
#include "glm/mat4x4.hpp"

void log(const char *format, ...);

void log_mat4(const char *pre, const glm::mat4 &mat);

/**
 * Assumes res/ is in working dir or at env var VOXELS_PATH
 *
 * @param out
 * @param relative_path
 */
void resolve_resource_path(std::string &out, const char *relative_path);

#endif
