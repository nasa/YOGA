#pragma once
#include <parfait/CartBlock.h>

namespace YOGA{
inline Parfait::CartBlock generateCartBlock(const Parfait::Extent<double>& e, int max_cells) {
    double dxdy = e.getLength_X() / e.getLength_Y();
    double dxdz = e.getLength_X() / e.getLength_Z();
    int nx, ny, nz;
    nx = std::max(1, int(std::cbrt(max_cells)));
    ny = std::max(1, int(nx / dxdy));
    nz = std::max(1, int(nx / dxdz));
    while (nx * ny * nz <= max_cells) {
        nx++;
        ny = std::max(1, int(nx / dxdy));
        nz = std::max(1, int(nx / dxdz));
    }
    nx--;
    ny = std::max(1, int(nx / dxdy));
    nz = std::max(1, int(nx / dxdz));
    return {e, nx, ny, nz};
}
}