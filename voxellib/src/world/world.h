#ifndef VOXELS_WORLD_H
#define VOXELS_WORLD_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "glm/vec3.hpp"
#include "chunk.h"
#include "centre.h"
#include "loader.h"
class Camera;

class World {
public:
    World(glm::vec3 spawn_pos = {0, 0, 0}, glm::vec3 spawn_dir = {1, 0, 0});

    ~World();

    void register_camera(Camera *camera);

    void tick();

    void clear_all_chunks();

    void tweak_loaded_chunk_radius(int delta);

    inline int loaded_chunk_radius() const { return loader_.loaded_chunk_radius(); }

    inline int loaded_chunk_count() const { return loader_.loaded_chunk_radius_chunk_count(); }

    inline ChunkMap::RenderableChunkIterator renderable_chunks() const { return loader_.renderable_chunks(); }

//    static bool is_in_loaded_range(int centre_x, int centre_z, int load_radius, int chunk_x, int chunk_z);

private:
    std::unordered_set<ChunkId_t> per_frame_chunks_; // used to find which chunks should be unloaded

    WorldCentre centre_;

    struct {
        glm::vec3 position_;
        glm::vec3 direction_;
    } spawn_;

    WorldLoader loader_;

    void update_active_chunks();
};


#endif
