#include "block.h"

constexpr unsigned long rgba(unsigned int r, unsigned int g, unsigned int b, unsigned int a = 255) {
    return (((a & 0xFFu) << 24u) +
            ((b & 0xFFu) << 16u) +
            ((g & 0xFFu) << 8u) +
            ((r & 0xFFu) << 0u));
}

unsigned int BlockType::colour() const {
    switch (value_) {
        case kAir:
            return rgba(255, 255, 255);
        case kGrass:
            return rgba(113, 170, 52);
        case kStone:
            return rgba(125,112,113);
        case kDarkStone:
            return rgba(90, 83, 83);
        case kMarker:
            return rgba(230, 72, 46);
    }

    assert(false);
    return 0;
}

bool BlockType::opaque() const {
    return value_ != BlockType::kAir;
}
