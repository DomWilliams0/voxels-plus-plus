#ifndef VOXELS_CONSTANTS_H
#define VOXELS_CONSTANTS_H

const double kCameraTurnSpeed = 0.001;
const float kCameraMoveSpeed = 40.0f;

/**
* blocks per m
*/
const int kBlockScale = 2;

// /2 again because radius, not diameter
const float kBlockRadius = 1.f / kBlockScale / 2.f;

const unsigned int kChunkWidthShift = 5; // 32
const unsigned int kChunkHeightShift = 6; // 64 TODO change to 10 = 1024
const unsigned int kChunkDepthShift = 5; // 32

const int kChunkWidth = 1U << kChunkWidthShift;
const int kChunkHeight = 1U << kChunkHeightShift;
const int kChunkDepth = 1U << kChunkDepthShift;

const unsigned int kBlocksPerChunk = kChunkWidth * kChunkHeight * kChunkDepth;


// as defined in kBlockVertices, 3 for pos
const int kFloatsPerVertex = 3;

/**
 * vec3f (pos) + vec4b (colour) + float (ao)
 */
const int kChunkMeshWordsPerVertexInstance = 3 + 1 + 1;

/**
 * 12 triangles
 */
const int kChunkMeshVerticesPerBlock = 12 * 3;

const int kChunkMeshSize = kBlocksPerChunk * kChunkMeshWordsPerVertexInstance * kChunkMeshWordsPerVertexInstance;

#endif
