#ifndef VOXELS_STATE_H
#define VOXELS_STATE_H

#include <cstdint>
#include <string>

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

    constexpr bool is_loading() const { return value_ != kUnloaded && value_ != kCached; }

    std::string str() const;

    constexpr Value operator*() const {return value_;}


private:
    Value value_;
};


#endif
