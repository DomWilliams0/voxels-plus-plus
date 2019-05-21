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

    // free unloaded chunks
    loader_.clear_garbage();
}

constexpr std::array<int, kLoadedChunkRadiusChunkCount * 2> mk_coord_spiral() {
    auto array = std::array<int, kLoadedChunkRadiusChunkCount * 2>();

    int layer = 1, leg = 0, x = 0, z = 0;
    for (int i = 0; i < kLoadedChunkRadiusChunkCount; ++i) {
        // write coord out
        array[(i * 2) + 0] = x;
        array[(i * 2) + 1] = z;

        // advance in spiral
        switch (leg) {
            case 0:
                ++x;
                if (x == layer) ++leg;
                break;
            case 1:
                ++z;
                if (z == layer) ++leg;
                break;
            case 2:
                --x;
                if (-x == layer) ++leg;
                break;
            case 3:
                --z;
                if (-z == layer) {
                    leg = 0;
                    ++layer;
                }
                break;
        }
    }

    return array;
}

constexpr std::array<int, kLoadedChunkRadiusChunkCount * 2> kLoadedChunkRadiusSpiral = mk_coord_spiral();

void World::update_active_chunks() {
    ChunkEntry entry;

    ChunkId_t centre_chunk;
    if (!centre_.chunk(centre_chunk) && !chunks_.empty()) {
        // centre chunk has not changed, do nothing
        return;
    }

    per_frame_chunks_.clear();

    int centre_x, centre_z;
    ChunkId_deconstruct(centre_chunk, centre_x, centre_z);

    // make sure all chunks in range are loaded/loading
    // iterate in outward spiral
    for (int i = 0; i < kLoadedChunkRadiusChunkCount * 2; i += 2) {
        int x = centre_x - kLoadedChunkRadiusSpiral[i + 0];
        int z = centre_z - kLoadedChunkRadiusSpiral[i + 1];

        ChunkId_t c = ChunkId(x, z);

        // mark this chunk as in range
        per_frame_chunks_.insert(c);

        find_chunk(c, entry);

        // if unloaded, need to load
        if (entry.state == ChunkState::kUnloaded) {
            // does not block
            loader_.request_chunk(c);
            continue;
        }
    }

    // unload chunks out of range
    for (auto it = chunks_.begin(); it != chunks_.end();) {
        if (it->second.state == ChunkState::kLoaded && per_frame_chunks_.find(it->first) == per_frame_chunks_.end()) {
            // loaded and not in range
            Chunk *chunk = it->second.chunk;
            loader_.unload_chunk(chunk); // takes ownership

            it = chunks_.erase(it);
        } else {
            it++;
        }
    }

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
        loader_.unload_chunk(chunk);
    }
    chunks_.clear();

    NativeGenerator::mark_dirty();
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


World::~World() {
    clear_all_chunks();
}
