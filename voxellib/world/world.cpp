#include <GL/glew.h>
#include "world.h"

World::World() {

}

void World::add_loaded_chunk(Chunk *chunk) {
    if (chunk == nullptr || !chunk->loaded()) throw std::runtime_error("chunk is not loaded!");

    ChunkEntry entry = {.state = ChunkState::kLoaded, .chunk = chunk};
    chunks_.insert(std::make_pair(chunk->id(), entry));

}

World::RenderableChunkIterator World::renderable_chunks() {
    return World::RenderableChunkIterator(chunks_);
}


bool World::RenderableChunkIterator::next(Chunk **out) {
    while (iterator_ != end_) {
        auto &pair = *iterator_;
        Chunk *chunk = pair.second.chunk;

        // not renderable
        if (!ChunkState_renderable(pair.second.state))
            goto next;

        // lazily generate vao and vbo
        chunk->lazily_init_render_buffers();

        // return this chunk
        *out = chunk;
        return true;

        next:
        iterator_++;
    }

    // all done
    return false;
}
