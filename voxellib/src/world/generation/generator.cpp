#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <cstdint>
#include <cstring>
#include <boost/thread/locks.hpp>
#include <dlfcn.h>

#include "error.h"
#include "util.h"
#include "generator.h"

#include <boost/thread/locks.hpp>

int IGenerator::generate(ChunkId_t chunk_id, int seed, Chunk *chunk) {
    return generate(chunk_id, seed, chunk->terrain_);
}

int DummyGenerator::generate(ChunkId_t chunk_id, int seed, ChunkTerrain &terrain_out) {
    UNUSED(chunk_id);
    UNUSED(seed);

    // ground
    for (size_t x = 0; x < kChunkWidth; ++x) {
        for (size_t z = 0; z < kChunkDepth; ++z) {
            for (size_t y = 0; y < 1; ++y) {
                Block &b = terrain_out[{x, y, z}];
                b.type_ = x == 0 || x == kChunkWidth - 1 ||
                          z == 0 || z == kChunkDepth - 1 ? BlockType::kGrass : BlockType::kStone;
            }
        }
    }

    // TODO some seeded features


    return kErrorSuccess;
}

static size_t recv_all(int s, void *buf, size_t len) {
    size_t cur = 0;
    size_t total = 0;
    const int CHUNK_SIZE = 1U << 16U;

    while (total < len) {
        auto n = recv(s, (char *) buf + cur, CHUNK_SIZE, 0);
        if (n <= 0)
            break;

        total += n;
        cur += n;
    }

    return total;
}

int PythonGenerator::generate(ChunkId_t chunk_id, int seed, ChunkTerrain &terrain_out) {
    // next time you visit this you'll think goddamn what a mess and either you'll redo it or decide the
    // python generator is not worth it and delete it

    bool is_first_try;

    is_first_try = true;
    goto open_and_go;

    try_again:
    is_first_try = false;

    // open socket
    open_and_go:
    int ret;
    if ((ret = get_socket()) != kErrorSuccess)
        return ret;

    // send request
    struct {
        int32_t version, cw, ch, cd, x, z, seed;
    } req;

    const int kVersion = 1;
    req.version = kVersion;
    req.cw = kChunkWidth;
    req.ch = kChunkHeight;
    req.cd = kChunkDepth;
    ChunkId_deconstruct(chunk_id, req.x, req.z);
    req.seed = seed;

    int32_t buf[kBlocksPerChunk] = {0};
    size_t n_expected = kBlocksPerChunk * sizeof(int32_t);
    size_t n = send(sock_, &req, sizeof(req), 0);

    if (n != sizeof(req)) {
        close(sock_);
        sock_ = -1;
        if (!is_first_try)
            return 4; // uh oh

        DLOG_F(WARNING, "socket have been closed, trying again");
        goto try_again;
    }

    n = recv_all(sock_, buf, n_expected);
    if (n != n_expected) {
        LOG_F(ERROR, "only got %zu/%zu bytes", n, n_expected);
        return 5; // TODO error
    }

    for (unsigned int i = 0; i < kBlocksPerChunk; i++) {
        Block &b = terrain_out[i];
        b.type_ = static_cast<BlockType>(buf[i]);
    }


    return 0;
}

int PythonGenerator::get_socket() {
    if (sock_ == -1) {
        struct sockaddr_un server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sun_family = AF_UNIX;
        strcpy(server_address.sun_path, "/tmp/voxels-gen");
        if ((sock_ = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            LOG_F(ERROR, "could not create socket: %d", errno);
            return kErrorIo;
        }

        if (connect(sock_, (struct sockaddr *) &server_address,
                    sizeof(server_address)) < 0) {
            LOG_F(ERROR, "could not connect to server: %d", errno);
            return kErrorIo;
        }
    }

    return kErrorSuccess;
}

#ifndef PROCGEN_BIN
#error missing PROCGEN_BIN
#endif

boost::shared_mutex NativeGenerator::kHandleMutex;
void *NativeGenerator::kHandle;
generate_t NativeGenerator::kFunc;
bool NativeGenerator::kDirty;

int NativeGenerator::ensure_handle() {
    boost::upgrade_lock lock(kHandleMutex);
    if (kDirty || kFunc == nullptr) {
        boost::upgrade_to_unique_lock unique_lock(lock);

        if (kHandle != nullptr)
            dlclose(kHandle);

        kHandle = dlopen(PROCGEN_BIN, RTLD_NOW);
        if (kHandle == nullptr) {
            LOG_F(ERROR, "failed to dlopen: %s", dlerror());
            return kErrorDl;
        }

        kFunc = (generate_t) dlsym(kHandle, "generate");
        if (kFunc == nullptr) {
            LOG_F(ERROR, "failed to dlsym: %s", dlerror());
            return kErrorDl;
        }

        LOG_F(INFO, "reloaded native generator");
        kDirty = false;
    }

    return kErrorSuccess;
}

int NativeGenerator::generate(ChunkId_t chunk_id, int seed, ChunkTerrain &terrain_out) {
    int x, z;
    ChunkId_deconstruct(chunk_id, x, z);

    int ret;
    if ((ret = ensure_handle()) != kErrorSuccess)
        return ret;

    {
        // hold lock while inside function
        boost::shared_lock<boost::shared_mutex> lock(kHandleMutex);
        return kFunc(x, z, seed, terrain_out);
    }
}

void NativeGenerator::mark_dirty() {
    boost::unique_lock<boost::shared_mutex> lock(kHandleMutex);
    kDirty = true;
}
