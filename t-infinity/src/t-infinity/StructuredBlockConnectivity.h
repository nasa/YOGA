#pragma once

#include <map>
#include "StructuredMesh.h"
#include <MessagePasser/MessagePasser.h>
#include "StructuredBlockFace.h"

namespace inf {

BlockFace getBlockFaces(const StructuredBlock& block, BlockSide side);
bool blockFacesMatch(const BlockFaceCorners& a, const BlockFaceCorners& b);

constexpr BlockAxis sideToAxis(BlockSide side) { return static_cast<BlockAxis>(side / 2); }
constexpr bool isMaxFace(BlockSide side) { return (side % 2 == 1); }
constexpr BlockSide getOppositeSide(BlockSide side) { return static_cast<BlockSide>(side ^ 0b001); }
std::array<int, 3> shiftIJK(std::array<int, 3> ijk, BlockSide side, int distance);

namespace TangentAxis {
    BlockAxis sideToFirst(BlockSide side);
    BlockAxis sideToSecond(BlockSide side);
}
bool pointsMatchToRelativeTolerance(const Parfait::Point<double>& p1,
                                    const Parfait::Point<double>& p2,
                                    double tol);
auto matchFaceAxis(BlockSide host, BlockSide neighbor) -> FaceAxisMapping;
auto getTangentFaceMapping(int corner1, int corner2, BlockSide neighbor_side) -> FaceAxisMapping;
auto findCorner(const Parfait::Point<double>& p, const BlockFaceCorners& corners) -> int;
auto getFaceAxisMapping(const BlockFace& host, const BlockFace& neighbor)
    -> std::array<FaceAxisMapping, 3>;
auto getFaceConnectivity(const BlockFace& host, const BlockFace& neighbor) -> FaceConnectivity;
auto matchFaceTangentAxis(const Parfait::Point<double>& p1, const Parfait::Point<double>& p2, const BlockFace& neighbor)
    -> FaceAxisMapping;
auto getCornerPointDistanceTolerance(const BlockFaceCorners& corners) -> double;
}
