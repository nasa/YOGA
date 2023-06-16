#pragma once
#include <stdint.h>
#include <limits.h>
#include "Point.h"
#include "Extent.h"

namespace Parfait {
PARFAIT_INLINE uint64_t mortonEncode_for(unsigned int x, unsigned int y, unsigned int z) {
    uint64_t answer = 0;
    for (uint64_t i = 0; i < (sizeof(uint64_t) * CHAR_BIT) / 3; ++i) {
        answer |= ((x & ((uint64_t)1 << i)) << 2 * i) | ((y & ((uint64_t)1 << i)) << (2 * i + 1)) |
                  ((z & ((uint64_t)1 << i)) << (2 * i + 2));
    }
    return answer;
}

// method to seperate bits from a given integer 3 positions apart
PARFAIT_INLINE uint64_t splitBy3(unsigned int a) {
    uint64_t x = a & 0x1fffff;             // we only look at the first 21 bits
    x = (x | x << 32) & 0x1f00000000ffff;  // shift left 32 bits, OR with self, and
    // 00011111000000000000000000000000000000001111111111111111
    x = (x | x << 16) & 0x1f0000ff0000ff;  // shift left 32 bits, OR with self, and
    // 00011111000000000000000011111111000000000000000011111111
    x = (x | x << 8) & 0x100f00f00f00f00f;  // shift left 32 bits, OR with self, and
    // 0001000000001111000000001111000000001111000000001111000000000000
    x = (x | x << 4) & 0x10c30c30c30c30c3;  // shift left 32 bits, OR with self, and
    // 0001000011000011000011000011000011000011000011000011000100000000
    x = (x | x << 2) & 0x1249249249249249;
    return x;
}

PARFAIT_INLINE uint64_t mortonEncode_magicbits(unsigned int x, unsigned int y, unsigned int z) {
    uint64_t answer = 0;
    answer |= splitBy3(x) | splitBy3(y) << 1 | splitBy3(z) << 2;
    return answer;
}

PARFAIT_INLINE void DecodeMorton(const unsigned long int morton, unsigned int& x, unsigned int& y, unsigned int& z) {
    x = 0;
    y = 0;
    z = 0;
    for (uint64_t i = 0; i < (sizeof(uint64_t) * CHAR_BIT) / 3; ++i) {
        x |= ((morton & (uint64_t(1ull) << uint64_t((3ull * i) + 0ull))) >> uint64_t(((3ull * i) + 0ull) - i));
        y |= ((morton & (uint64_t(1ull) << uint64_t((3ull * i) + 1ull))) >> uint64_t(((3ull * i) + 1ull) - i));
        z |= ((morton & (uint64_t(1ull) << uint64_t((3ull * i) + 2ull))) >> uint64_t(((3ull * i) + 2ull) - i));
    }
}

class MortonID {
  public:
    PARFAIT_INLINE static Parfait::Point<double> getPointFromMortonID(uint64_t mortonID,
                                                                      const Parfait::Extent<double>& domain);
    PARFAIT_INLINE static Parfait::Point<double> getPointFromMortonCoords(uint32_t i,
                                                                          uint32_t j,
                                                                          uint32_t k,
                                                                          const Parfait::Extent<double>& root_domain);
    PARFAIT_INLINE static uint64_t getMortonIdFromPoint(const Parfait::Extent<double>& domain,
                                                        const Parfait::Point<double>& p);
};
PARFAIT_INLINE Parfait::Point<double> MortonID::getPointFromMortonCoords(uint32_t i,
                                                                         uint32_t j,
                                                                         uint32_t k,
                                                                         const Parfait::Extent<double>& root_domain) {
    unsigned int max_side_cells = 2097152;  // 2^21 maximum size allowed by the morton procedures.
    double x_ratio = (double)i / (double)max_side_cells;
    double y_ratio = (double)j / (double)max_side_cells;
    double z_ratio = (double)k / (double)max_side_cells;

    double x_length = root_domain.hi[0] - root_domain.lo[0];
    double y_length = root_domain.hi[1] - root_domain.lo[1];
    double z_length = root_domain.hi[2] - root_domain.lo[2];

    Parfait::Point<double> p;
    p[0] = root_domain.lo[0] + x_ratio * x_length;
    p[1] = root_domain.lo[1] + y_ratio * y_length;
    p[2] = root_domain.lo[2] + z_ratio * z_length;

    return p;
}

PARFAIT_INLINE Parfait::Point<double> MortonID::getPointFromMortonID(uint64_t mortonID,
                                                                     const Parfait::Extent<double>& root_domain) {
    unsigned int i, j, k;
    DecodeMorton(mortonID, i, j, k);

    return getPointFromMortonCoords(i, j, k, root_domain);
}
PARFAIT_INLINE uint64_t MortonID::getMortonIdFromPoint(const Parfait::Extent<double>& domain,
                                                       const Parfait::Point<double>& p) {
    unsigned int max_side_cells = 2097152;  // 2^21 maximum size allowed by the morton procedures.

    double dx = p[0] - domain.lo[0];
    double dy = p[1] - domain.lo[1];
    double dz = p[2] - domain.lo[2];
    double x_ratio = dx / (domain.hi[0] - domain.lo[0]);
    double y_ratio = dy / (domain.hi[1] - domain.lo[1]);
    double z_ratio = dz / (domain.hi[2] - domain.lo[2]);

    uint32_t i = std::floor(x_ratio * max_side_cells);
    uint32_t j = std::floor(y_ratio * max_side_cells);
    uint32_t k = std::floor(z_ratio * max_side_cells);
    return mortonEncode_magicbits(i, j, k);
}
}
