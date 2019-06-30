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

    void register_camera(Camera *camera);

    // slightly temporary
    void stop_chunk_loading();

    void tick();

    void clear_all_chunks();

    void tweak_loaded_chunk_radius(int delta);

    inline unsigned int loaded_chunk_radius() const { return loaded_chunk_radius_; }

    inline void get_renderable_chunks(std::vector<ChunkMesh *> &out) { loader_->get_renderable_chunks(out); }

    inline void finished_rendering() { loader_->finished_rendering(); }

    inline void get_gl_goshdarn_garbage(std::vector<WorldLoader::GlGarbage> &out) {
        loader_->get_gl_goshdarn_garbage(out);
    }

    // gracefully stop all threads
    void cleanup();

private:
    WorldCentre centre_ {};
    unsigned int loaded_chunk_radius_;

    struct {
        glm::vec3 position_ {};
        glm::vec3 direction_ {};
    } spawn_;

    WorldLoader *loader_;
    boost::thread *loader_thread_;
};


#endif
