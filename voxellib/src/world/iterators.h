#ifndef VOXELS_ITERATORS_H
#define VOXELS_ITERATORS_H

#define ITERATOR_CHUNK_SPIRAL_BEGIN(loaded_chunk_radius_chunk_count, centre_x, centre_z) do {\
    int layer = 1, leg = 0, _x = 0, _z = 0; \
    for (int _i = 0; _i < (loaded_chunk_radius_chunk_count); ++_i) { \
        /* use coords */ \
        int x = (centre_x) - _x; \
        int z = (centre_z) - _z; \
        /* user code goes here */

#define ITERATOR_CHUNK_SPIRAL_END \
        /* advance in spiral */ \
        switch (leg) { \
            case 0: \
                ++_x; \
                if (_x == layer) ++leg; \
                break; \
            case 1: \
                ++_z; \
                if (_z == layer) ++leg; \
                break; \
            case 2: \
                --_x; \
                if (-_x == layer) ++leg; \
                break; \
            case 3: \
                --_z; \
                if (-_z == layer) { \
                    leg = 0; \
                    ++layer; \
                } \
                break; \
        } \
    } \
} while (0);

#endif
