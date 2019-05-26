#ifndef VOXELS_STATE_H
#define VOXELS_STATE_H


#include <cstdint>

class ChunkState {
public:
    enum Value : uint8_t {
        kUnloaded,
        kLoadingTerrain,
        kLoadedTerrain,
        kRenderable,
        kCached,
    };

    ChunkState() = default;

    constexpr ChunkState(Value val) : value_(val) {}

    constexpr bool operator==(ChunkState o) const { return value_ == o.value_; }

    constexpr bool operator!=(ChunkState o) const { return value_ != o.value_; }

    constexpr bool is_loaded() const { return value_ != kUnloaded && value_ != kCached; }

private:
    Value value_;
};


#endif
