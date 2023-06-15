#include "StructuredBlockConnectivity.h"
#include <parfait/Throw.h>

using namespace inf;

inf::BlockFace inf::getBlockFaces(const inf::StructuredBlock& block, inf::BlockSide side) {
    auto dimensions = block.nodeDimensions();
    int imin = 0;
    int jmin = 0;
    int kmin = 0;
    int imax = dimensions[0] - 1;
    int jmax = dimensions[1] - 1;
    int kmax = dimensions[2] - 1;
    BlockFace face{};
    face.side = side;
    face.block_id = block.blockId();

    switch (side) {
        case IMIN:
            face.corners[0] = block.point(imin, jmin, kmin);
            face.corners[1] = block.point(imin, jmax, kmin);
            face.corners[2] = block.point(imin, jmax, kmax);
            face.corners[3] = block.point(imin, jmin, kmax);
            break;
        case IMAX:
            face.corners[0] = block.point(imax, jmin, kmin);
            face.corners[1] = block.point(imax, jmax, kmin);
            face.corners[2] = block.point(imax, jmax, kmax);
            face.corners[3] = block.point(imax, jmin, kmax);
            break;
        case JMIN:
            face.corners[0] = block.point(imin, jmin, kmin);
            face.corners[1] = block.point(imax, jmin, kmin);
            face.corners[2] = block.point(imax, jmin, kmax);
            face.corners[3] = block.point(imin, jmin, kmax);
            break;
        case JMAX:
            face.corners[0] = block.point(imin, jmax, kmin);
            face.corners[1] = block.point(imax, jmax, kmin);
            face.corners[2] = block.point(imax, jmax, kmax);
            face.corners[3] = block.point(imin, jmax, kmax);
            break;
        case KMIN:
            face.corners[0] = block.point(imin, jmin, kmin);
            face.corners[1] = block.point(imax, jmin, kmin);
            face.corners[2] = block.point(imax, jmax, kmin);
            face.corners[3] = block.point(imin, jmax, kmin);
            break;
        case KMAX:
            face.corners[0] = block.point(imin, jmin, kmax);
            face.corners[1] = block.point(imax, jmin, kmax);
            face.corners[2] = block.point(imax, jmax, kmax);
            face.corners[3] = block.point(imin, jmax, kmax);
            break;
        default:
            throw std::runtime_error("Invalid side: " + std::to_string(side));
    }
    return face;
}
bool inf::blockFacesMatch(const inf::BlockFaceCorners& a, const inf::BlockFaceCorners& b) {
    auto min_distance_a = getCornerPointDistanceTolerance(a); 
    auto min_distance_b = getCornerPointDistanceTolerance(b); 
    double min_distance = std::min(min_distance_a, min_distance_b);
    for (const auto& p : a) {
        bool found_match = false;
        for (const auto& p2 : b) {
            if (pointsMatchToRelativeTolerance(p, p2, 0.01 * min_distance)) {
                found_match = true;
                break;
            }
        }
        if (not found_match) return false;
    }
    return true;
}
inf::FaceAxisMapping inf::matchFaceAxis(inf::BlockSide host, inf::BlockSide neighbor) {
    auto neighbor_axis = sideToAxis(neighbor);
    auto host_max = isMaxFace(host);
    auto neighbor_max = isMaxFace(neighbor);
    FaceAxisMapping mapping{};
    mapping.axis = neighbor_axis;
    mapping.direction = (host_max xor neighbor_max) ? SameDirection : ReversedDirection;
    return mapping;
}
inf::FaceAxisMapping inf::getTangentFaceMapping(int corner1,
                                                int corner2,
                                                inf::BlockSide neighbor_side) {
    if ((corner1 == 0 and corner2 == 1) or (corner1 == 3 and corner2 == 2)) {
        return {TangentAxis::sideToFirst(neighbor_side), SameDirection};
    } else if ((corner1 == 0 and corner2 == 3) or (corner1 == 1 and corner2 == 2)) {
        return {TangentAxis::sideToSecond(neighbor_side), SameDirection};
    } else if ((corner1 == 0 and corner2 == 2) or (corner1 == 3 and corner2 == 1)) {
        PARFAIT_THROW("Block face is most likely twisted.");
    } else {
        FaceAxisMapping mapping = getTangentFaceMapping(corner2, corner1, neighbor_side);
        mapping.direction = ReversedDirection;
        return mapping;
    }
}
int inf::findCorner(const Parfait::Point<double>& p, const inf::BlockFaceCorners& corners) {
    int c = 0;
    auto min_distance = getCornerPointDistanceTolerance(corners);
    for (const auto& corner : corners) {
        if (pointsMatchToRelativeTolerance(corner, p, 0.01 * min_distance)) return c;
        c++;
    }
    PARFAIT_THROW("Unable to find matching corner");
}
inf::FaceAxisMapping inf::matchFaceTangentAxis(const Parfait::Point<double>& p1,
                                               const Parfait::Point<double>& p2,
                                               const inf::BlockFace& neighbor) {
    auto corner1 = findCorner(p1, neighbor.corners);
    auto corner2 = findCorner(p2, neighbor.corners);
    if (corner1 == corner2) {
        printf("p1: %s p2: %s\n", p1.to_string().c_str(), p2.to_string().c_str());
        PARFAIT_THROW("Points match, but should not.");
    }
    assert(not p1.approxEqual(p2));
    return getTangentFaceMapping(corner1, corner2, neighbor.side);
}
std::array<FaceAxisMapping, 3> inf::getFaceAxisMapping(const inf::BlockFace& host,
                                                       const inf::BlockFace& neighbor) {
    std::array<FaceAxisMapping, 3> mapping{};
    auto host_axis = sideToAxis(host.side);
    auto first_tangent_axis = TangentAxis::sideToFirst(host.side);
    auto second_tangent_axis = TangentAxis::sideToSecond(host.side);

    auto isDegenerate = [&](int corner1, int corner2) -> bool {
        return host.corners[corner1].approxEqual(host.corners[corner2]);
    };

    mapping[host_axis] = matchFaceAxis(host.side, neighbor.side);
    if (isDegenerate(0, 1)) {
        mapping[first_tangent_axis] =
            matchFaceTangentAxis(host.corners[3], host.corners[2], neighbor);
    } else {
        mapping[first_tangent_axis] =
            matchFaceTangentAxis(host.corners[0], host.corners[1], neighbor);
    }
    if (isDegenerate(0, 3)) {
        mapping[second_tangent_axis] =
            matchFaceTangentAxis(host.corners[1], host.corners[2], neighbor);
    } else {
        mapping[second_tangent_axis] =
            matchFaceTangentAxis(host.corners[0], host.corners[3], neighbor);
    }

    return mapping;
}
inf::FaceConnectivity inf::getFaceConnectivity(const inf::BlockFace& host,
                                               const inf::BlockFace& neighbor) {
    return {getFaceAxisMapping(host, neighbor), neighbor.block_id, neighbor.side};
}

bool inf::pointsMatchToRelativeTolerance(const Parfait::Point<double>& p1,
                                         const Parfait::Point<double>& p2,
                                         double tol) {
    for (int direction = 0; direction < 3; ++direction) {
        double tolerance = std::min(std::abs(p1[direction]), std::abs(p2[direction])) * tol;
        tolerance = std::max(tolerance, 1e-14);
        if (std::isnan(p1[direction]) or std::isinf(p1[direction])) return false;
        if (p1[direction] + tolerance < p2[direction]) return false;
        if (p1[direction] - tolerance > p2[direction]) return false;
    }
    return true;
}
std::array<int, 3> inf::shiftIJK(std::array<int, 3> ijk, BlockSide side, int distance) {
    auto axis = sideToAxis(side);
    ijk[axis] += isMaxFace(side) ? distance : -distance;
    return ijk;
}

//  First axis: I -> J ; J -> I ; K -> I
// Second axis: I -> K ; J -> K ; K -> J
BlockAxis TangentAxis::sideToFirst(BlockSide side) { return side == IMIN or side == IMAX ? J : I; }
BlockAxis TangentAxis::sideToSecond(BlockSide side) { return side == KMIN or side == KMAX ? J : K; }

double inf::getCornerPointDistanceTolerance(const inf::BlockFaceCorners& corners) {
    double min_distance = std::numeric_limits<double>::max();
    for (const auto& corner1 : corners) {
        for (const auto& corner2 : corners) {
            auto distance = corner1.distance(corner2);
            if (distance > 0.0 and distance < min_distance) min_distance = distance;
        }
    }
    return min_distance;
}