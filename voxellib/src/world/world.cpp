#include <GL/glew.h>
#include <error.h>
#include "world.h"
#include "camera.h"
#include "util.h"
#include "loader.h"

World::World(glm::vec3 spawn_pos, glm::vec3 spawn_dir) : spawn_{.position_=spawn_pos, .direction_=spawn_dir},
                                                         loader_(50) /* TODO random */ {}

void World::add_loaded_chunk(Chunk *chunk) {
    if (chunk == nullptr || !chunk->loaded()) throw std::runtime_error("chunk is not loaded!");

    ChunkEntry entry = {.state = ChunkState::kLoaded, .chunk = chunk};
    chunks_.insert(std::make_pair(chunk->id(), entry));
}

World::RenderableChunkIterator World::renderable_chunks() {
    return World::RenderableChunkIterator(chunks_);
}

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

    // load complete chunks
    Chunk *done_chunk = nullptr;
    while (loader_.pop_done(done_chunk)) {
        int x, z;
        ChunkId_deconstruct(done_chunk->id(), x, z);
//        log("recv loaded chunk(%d, %d)", x, z);
        add_loaded_chunk(done_chunk);
    }
}

void World::update_active_chunks() {
    ChunkEntry entry;

    ChunkId_t centre_chunk;
    if (!centre_.chunk(centre_chunk) && !chunks_.empty()) {
        // centre chunk has not changed, do nothing
        return;
    }

    int centre_x, centre_z;
    ChunkId_deconstruct(centre_chunk, centre_x, centre_z);

    for (int x = centre_x - kLoadedChunkRadius; x <= centre_x + kLoadedChunkRadius; ++x) {
        for (int z = centre_z - kLoadedChunkRadius; z <= centre_z + kLoadedChunkRadius; ++z) {
            ChunkId_t c = ChunkId(x, z);
            find_chunk(c, entry);

            // if unloaded, need to load
            if (entry.state == ChunkState::kUnloaded) {
                // does not block
                loader_.request_chunk(c);
                continue;
            }
        }
    }

    // TODO unload unneeded chunks
}

void World::find_chunk(ChunkId_t chunk_id, ChunkEntry &out) {
    auto it = chunks_.find(chunk_id);

    if (it == chunks_.end()) {
        out.state = ChunkState::kUnloaded;
    } else {
        out = it->second;
    }
}

void World::clear_all_chunks() {
    for (auto &entry : chunks_) {
        Chunk *chunk = entry.second.chunk;
        delete chunk;
    }
    chunks_.clear();
}


bool World::RenderableChunkIterator::next(Chunk **out) {
    while (iterator_ != end_) {
        auto &pair = *(iterator_++);

        // not renderable
        if (!ChunkState_renderable(pair.second.state))
            continue;

        Chunk *chunk = pair.second.chunk;

        // lazily generate vao and vbo
        chunk->lazily_init_render_buffers();

        // return this chunk
        *out = chunk;
        return true;
    }

    // all done
    return false;
}


