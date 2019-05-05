#include <GL/glew.h>
#include "world.h"

World::World() {
    // dummy
    Chunk *c = new Chunk(2, 1);

    // init terrain
    for (unsigned long w = 0; w < kChunkWidth; w++) {
        for (unsigned long h = 0; h < kChunkHeight; h++) {
            for (unsigned long d = 0; d < kChunkDepth; d++) {
                Block &b = c->terrain_[{w, h, d}];
                b.type = BlockType::kThing;
            }
        }
    }

    // generate mesh
    c->generate_mesh();

    // register
    add_loaded_chunk(c);
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
