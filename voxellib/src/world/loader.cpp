#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "error.h"
#include "util.h"
#include "loader.h"
#include "config.h"

static void update_face_visibility(ChunkTerrain &terrain) {
    glm::ivec3 pos;
    for (int i = 0; i < kBlocksPerChunk; ++i) {
        Chunk::expand_block_index(terrain, i, pos);
        Block &b = terrain[i];
        FaceVisibility visibility = b.face_visibility_;

        if (!BlockType_opaque(b.type_)) {
            // fully visible because transparent
            visibility = kFaceVisibilityAll;
        } else {
            // check each face individually
            glm::ivec3 offset_pos;
            for (int j = 0; j < kFaceCount; ++j) {
                offset_pos = pos; // reset
                Face face = kFaces[j];
                face_offset(face, offset_pos);

                // looks outside of the world, so this face is visible
                // TODO YEAH?!
                if (offset_pos.y < 0 || offset_pos.y >= kChunkHeight) {
                    visibility |= face_visibility(face);
                    continue;
                }

                // steps over chunk boundary
                if (offset_pos.x < 0 || offset_pos.x >= kChunkWidth ||
                    offset_pos.z < 0 || offset_pos.z >= kChunkDepth) {
                    // TODO actually get it
                    // edge, never mind
                    visibility |= face_visibility(face);
                    continue;
                }

                // inside this chunk, safe to get the block type (rather ugly...)
                Block &offset_block = terrain[{static_cast<unsigned long>(offset_pos.x),
                                               static_cast<unsigned long>(offset_pos.y),
                                               static_cast<unsigned long>(offset_pos.z)}];

                if (BlockType_opaque(offset_block.type_))
                    visibility &= ~face_visibility(face); // not visible
                else
                    visibility |= face_visibility(face); // visible
            }
        }

        b.face_visibility_ = visibility;
    }

}


WorldLoader::WorldLoader(int seed) : seed_(seed), done_(32), pool_(config::kTerrainThreadWorkers) {}


void WorldLoader::request_chunk(ChunkId_t chunk_id) {
    // TODO correct to capture this?
    boost::asio::post(pool_, [this, chunk_id]() {
        int x, z;
        ChunkId_deconstruct(chunk_id, x, z);
//        DLOG_F(INFO, "about to load chunk(%d, %d)", x, z);

        auto chunk = new Chunk(x, z);

        // TODO load from cache/disk too
        // TODO delete when?
        thread_local IGenerator *gen = config::new_generator();

        int ret = gen->generate(chunk_id, seed_, chunk->terrain_);
        if (ret == kErrorSuccess) {
            // finalise terrain
            // TODO might already be populated, check first
            update_face_visibility(chunk->terrain_);
            chunk->generate_mesh();

            // add to done queue
            if (done_.push(chunk))
                return;

            LOG_F(WARNING, "failed to push complete chunk to done queue!");
        }

        LOG_F(WARNING, "failed to generate chunk(%d, %d) with seed %d: %d", x, z, seed_, ret);
        delete chunk;
    });

}

bool WorldLoader::pop_done(Chunk *&chunk_out) {
    return done_.pop(chunk_out);
}

void WorldLoader::unload_chunk(Chunk *chunk) {
    // TODO add to cache
    delete chunk;
}
