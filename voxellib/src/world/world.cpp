#include <GL/glew.h>
#include "error.h"
#include "config.h"
#include "world.h"
#include "camera.h"
#include "util.h"
#include "loader.h"
#include "generation/generator.h"

World::World(glm::vec3 spawn_pos, glm::vec3 spawn_dir) :
        loaded_chunk_radius_(config::kInitialLoadedChunkRadius) {
    spawn_.position_ = spawn_pos;
    spawn_.direction_ = spawn_dir;

    loader_ = WorldLoader::create(50, &loader_thread_);
}

void World::register_camera(Camera *camera) {
    // move to spawn position
    camera->set(spawn_.position_, spawn_.direction_);

    // follow
    centre_.follow(camera);
}

void World::stop_chunk_loading() {
    centre_.stop_following();
}

void World::tick() {
    // move world centre if necessary
    centre_.tick();

    ChunkId_t centre_chunk;
    centre_.chunk(centre_chunk);
    loader_->update_world_centre(centre_chunk, loaded_chunk_radius_);
}

void World::clear_all_chunks() {
    loader_->unload_all_chunks();
    NativeGenerator::mark_dirty();
}

void World::tweak_loaded_chunk_radius(int delta) {
    loaded_chunk_radius_ += delta;

    if (loaded_chunk_radius_ < 1)
        loaded_chunk_radius_ = 1;
    else
        LOG_F(INFO, "%s loaded chunk radius to %d", delta > 0 ? "bumped" : "reduced", loaded_chunk_radius_);

    centre_.reset();
}

void World::cleanup() {
    loader_->stop();
    loader_thread_->join();
    delete loader_thread_;
}
