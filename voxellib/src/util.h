#ifndef VOXELS_UTIL_H
#define VOXELS_UTIL_H

#include <string>
#include "loguru/loguru.hpp" // to avoid including in every damn file manually

/**
 * Assumes res/ is in working dir or at env var VOXELS_PATH
 *
 * @param out
 * @param relative_path
 */
void resolve_resource_path(std::string &out, const char *relative_path);

#endif
