#ifndef VOXELS_WORLD_H
#define VOXELS_WORLD_H

#include <vector>
#include <unordered_map>
#include "glm/vec3.hpp"
#include "chunk.h"
#include "centre.h"

enum class ChunkState {
    kUnloaded = 1,
    kLoaded,

};

inline bool ChunkState_renderable(const ChunkState &cs) {
    return cs == ChunkState::kLoaded; // TODO or mesh being updated
}

// radius around player to load chunks
const int kLoadedChunkRadius = 2;

// chunk map entry
struct ChunkEntry {
    ChunkState state;
    Chunk *chunk;
};

typedef std::unordered_map<ChunkId_t, ChunkEntry> ChunkMap;

class Camera;

class World {
public:
    World(glm::vec3 spawn_pos = {0, 0, 0}, glm::vec3 spawn_dir = {1, 0, 0});

    /**
     * @param chunk Must have terrain and mesh fully loaded
     */
    void add_loaded_chunk(Chunk *chunk);

    void register_camera(Camera *camera);

    void tick();


private:
    // TODO map of chunk id -> {load state, optional chunk *}
    // TODO use a hashset keyed with chunk id
    ChunkMap chunks_;

    WorldCentre centre_;

    struct {
        glm::vec3 position_;
        glm::vec3 direction_;
    } spawn_;

    void update_active_chunks();

    void find_chunk(ChunkId_t chunk_id, ChunkEntry &out);

    class RenderableChunkIterator {
    public:
        RenderableChunkIterator(const ChunkMap &chunks) : iterator_(chunks.cbegin()), end_(chunks.cend()) {}

        bool next(Chunk **out);

    private:
        ChunkMap::const_iterator iterator_;
        ChunkMap::const_iterator end_;
    };

public:
    World::RenderableChunkIterator renderable_chunks();
};


#endif
