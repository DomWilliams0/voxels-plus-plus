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

const int kChunkWidthShift = 4; // 16
const int kChunkHeightShift = 6; // 16 TODO change to 10 = 1024
const int kChunkDepthShift = 4; // 16

const int kChunkWidth = 1 << kChunkWidthShift;
const int kChunkHeight = 1 << kChunkHeightShift;
const int kChunkDepth = 1 << kChunkDepthShift;

const int kBlocksPerChunk = kChunkWidth * kChunkHeight * kChunkDepth;


// as defined in kBlockVertices, 3 for pos
const int kFloatsPerVertex = 3;

/**
 * vec3f (pos) + vec4b (colour)
 */
const int kChunkMeshWordsPerVertexInstance = 3 + 1 /*+ 1*/;

/**
 * 12 triangles
 */
const int kChunkMeshVerticesPerBlock = 12 * 3;

const int kChunkMeshSize = kBlocksPerChunk * kChunkMeshWordsPerVertexInstance * kChunkMeshWordsPerVertexInstance;

#endif
