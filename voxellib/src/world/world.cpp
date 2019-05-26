#include <GL/glew.h>
#include <error.h>
#include "world.h"
#include "camera.h"
#include "util.h"
#include "loader.h"
#include "iterators.h"
#include "generation/generator.h"

World::World(glm::vec3 spawn_pos, glm::vec3 spawn_dir) : spawn_{.position_=spawn_pos, .direction_=spawn_dir},
                                                         loader_(50) /* TODO random */ {}

void World::register_camera(Camera *camera) {
    // move to spawn position
    camera->set(spawn_.position_, spawn_.direction_);

    // follow
    centre_.follow(camera);
}

void World::tick() {
    // move world centre if necessary
    centre_.tick();

    // find chunks to load and unload based on world centre
    update_active_chunks();

    loader_.tick();
}

void World::update_active_chunks() {
    ChunkId_t centre_chunk;
    if (!centre_.chunk(centre_chunk) && !loader_.chunkmap().empty()) {
        // centre chunk has not changed, do nothing
        return;
    }

    per_frame_chunks_.clear();

    int centre_x, centre_z;
    ChunkId_deconstruct(centre_chunk, centre_x, centre_z);

    // make sure all chunks in range are loaded/loading
    // iterate in outward spiral
    ITERATOR_CHUNK_SPIRAL_BEGIN(loader_.loaded_chunk_radius_chunk_count(), centre_x, centre_z)
        ChunkId_t c = ChunkId(x, z);

        // mark this chunk as in range
        per_frame_chunks_.insert(c);

        ChunkState state;
        loader_.chunkmap().get_chunk(c, &state);
        if (!state.is_loading()) {
            loader_.request_chunk(c); // does not block
        }
    ITERATOR_CHUNK_SPIRAL_END

    // unload chunks out of range
    ChunkMap &chunks = loader_.chunkmap();
    for (ChunkMap::MapType::value_type &e : chunks) {
        Chunk *chunk = e.second;

        if (per_frame_chunks_.find(e.first) == per_frame_chunks_.end() &&
            chunk->get_state() == ChunkState::kRenderable) {

            // loaded and not in range
            loader_.unload_chunk(chunk);
        }
    }

}

void World::clear_all_chunks() {
    loader_.unload_all_chunks();
    NativeGenerator::mark_dirty();
}

void World::tweak_loaded_chunk_radius(int delta) {
    loader_.tweak_loaded_chunk_radius(delta);
    centre_.reset();
}

World::~World() {
    clear_all_chunks();
}

/*
bool World::is_in_loaded_range(int centre_x, int centre_z, int load_radius, int chunk_x, int chunk_z) {
    int xmax = centre_x + load_radius;
    int xmin = centre_x - load_radius;
    if (chunk_x < xmin || chunk_x > xmax)
        return false;

    int zmax = centre_z + load_radius;
    int zmin = centre_z - load_radius;
    if (chunk_z < zmin || chunk_z > zmax)
        return false;

    return true;
}
*/

