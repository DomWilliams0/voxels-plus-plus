#include "loader.h"

WorldLoader kWorldLoader(new DummyGenerator);

int WorldLoader::load(ChunkId_t chunk_id, ChunkTerrain *terrain_out) {
    // always generate from scratch for now
    return generator_->generate(chunk_id, terrain_out);
}

