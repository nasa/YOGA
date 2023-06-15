#include "OverlapMask.h"

namespace YOGA {
std::vector<bool> OverlapMask::buildNodeMask(const YogaMesh& m, MeshSystemInfo& info) {
    std::vector<bool> mask(m.nodeCount(), false);
    for (int i = 0; i < m.nodeCount(); ++i) {
        auto p = m.getNode<double>(i);
        if (isPointInOverlap(p, info)) mask[i] = true;
    }
    return mask;
}

bool OverlapMask::isPointInOverlap(const Parfait::Point<double>& p, MeshSystemInfo& info) {
    int numberOfPotentialOverlaps = 0;
    for (int i = 0; i < info.numberOfComponents(); ++i)
        if (info.getComponentExtent(i).intersects(p)) ++numberOfPotentialOverlaps;
    return numberOfPotentialOverlaps > 1;
}
}