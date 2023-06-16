#pragma once
#include <parfait/Point.h>

namespace inf {
enum BlockAxis { I = 0, J = 1, K = 2 };
enum AxisDirection { SameDirection = 0, ReversedDirection = 1 };
using BlockFaceCorners = std::array<Parfait::Point<double>, 4>;

enum BlockSide { IMIN = 0, IMAX = 1, JMIN = 2, JMAX = 3, KMIN = 4, KMAX = 5 };
constexpr std::array<BlockSide, 6> AllBlockSides = {IMIN, IMAX, JMIN, JMAX, KMIN, KMAX};

struct BlockFace {
    int block_id;
    BlockFaceCorners corners;
    BlockSide side;
};

struct FaceAxisMapping {
    BlockAxis axis;
    AxisDirection direction;
};

struct FaceConnectivity {
    std::array<FaceAxisMapping, 3> face_mapping;
    int neighbor_block_id;
    BlockSide neighbor_side;
};
}
