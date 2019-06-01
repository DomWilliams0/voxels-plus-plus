#include <cassert>
#include "state.h"

std::string ChunkState::str() const {
    switch (value_) {
        case kUnloaded:
            return "kUnloaded";
        case kLoadingTerrain:
            return "kLoadingTerrain";
        case kLoadedTerrain:
            return "kLoadedTerrain";
        case kRenderable:
            return "kRenderable";
        default:
            assert(false);
            return "";
    }
}
