#include "ParallelSurface.h"
#include "BoundaryConditions.h"

namespace YOGA {

std::vector<Parfait::Point<double>> ParallelSurface::getLocalSurfacePointsInComponent(const YogaMesh& m,
                                                                                      int component) {
    auto isNodeInThisSurface = generateNodeMask(m, component);
    std::vector<Parfait::Point<double>> myPoints;
    for (int i = 0; i < m.nodeCount(); ++i)
        if (isNodeInThisSurface[i]) myPoints.push_back(m.getNode<double>(i));
    return myPoints;
}

std::vector<std::vector<Parfait::Point<double>>> ParallelSurface::buildSurfaces(MessagePasser mp, const YogaMesh& m) {
    std::vector<std::vector<Parfait::Point<double>>> local_surfaces;
    int n = countComponents(mp, m);
    for (int i = 0; i < n; ++i) local_surfaces.push_back(ParallelSurface::getLocalSurfacePointsInComponent(m, i));
    std::vector<std::vector<Parfait::Point<double>>> surfaces(n);
    for (int i = 0; i < n; i++) mp.Gather(local_surfaces[i], surfaces[i]);
    return surfaces;
}

int ParallelSurface::countComponents(MessagePasser mp, const YogaMesh& m) {
    int maxId = 0;
    for (int i = 0; i < m.nodeCount(); ++i) maxId = std::max(maxId, m.getAssociatedComponentId(i));
    maxId = mp.ParallelMax(maxId, 0);
    mp.Broadcast(maxId, 0);
    return maxId + 1;
}

std::vector<bool> ParallelSurface::generateNodeMask(const YogaMesh& m, int component) {
    std::vector<bool> mask(m.nodeCount(), false);
    for (int i = 0; i < m.numberOfBoundaryFaces(); ++i) {
        auto bc = m.getBoundaryCondition(i);
        if (Solid == bc) {
            for (int nodeId : m.getNodesInBoundaryFace(i))
                if (m.getAssociatedComponentId(nodeId) == component) mask[nodeId] = true;
        }
    }
    return mask;
}
}
