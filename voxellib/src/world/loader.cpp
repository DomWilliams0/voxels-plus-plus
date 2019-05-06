#include <error.h>
#include <util.h>
#include "loader.h"

WorldLoader kWorldLoader(new DummyGenerator);

static void update_face_visibility(ChunkTerrain *terrain) {
    Block *b;
    glm::ivec3 pos;
    for (int i = 0; i < kBlocksPerChunk; ++i) {
        Chunk::expand_block_index(terrain, i, pos);
        b = &terrain->operator[](i);
        FaceVisibility visibility = b->face_visibility_;

        if (!BlockType_opaque(b->type_)) {
            // fully visible because transparent
            visibility = kFaceVisibilityAll;
        } else {
            // check each face individually
            glm::ivec3 offset_pos;
            Block *offset_block;
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
                offset_block = &terrain->operator[]({static_cast<unsigned long>(offset_pos.x),
                                                     static_cast<unsigned long>(offset_pos.y),
                                                     static_cast<unsigned long>(offset_pos.z)});

                if (BlockType_opaque(offset_block->type_))
                    visibility &= ~face_visibility(face); // not visible
                else
                    visibility |= face_visibility(face); // visible
            }
        }

        b->face_visibility_ = visibility;
    }

}


int WorldLoader::load(ChunkId_t chunk_id, ChunkTerrain *terrain_out) {
    // always generate from scratch for now

    int ret = generator_->generate(chunk_id, terrain_out);
    if (ret == kErrorSuccess) {
        // populate face visibility
        // TODO this might be populated already, do a check
        update_face_visibility(terrain_out);
    }
    return ret;
}

