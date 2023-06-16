#pragma once
#include "MeshInterface.h"
#include "MeshConnectivity.h"
#include "Cell.h"

namespace inf {
std::vector<double> calcSurfaceSmoothness(const MeshInterface& m) {
    auto n2c = NodeToCell::buildSurfaceOnly(m);
    std::vector<double> smallest_dot(m.nodeCount(), std::numeric_limits<double>::max());
    for (int n = 0; n < m.nodeCount(); n++) {
        for (int neighbor_1 : n2c[n]) {
            auto normal1 = inf::Cell(m, neighbor_1).faceAreaNormal(0);
            normal1.normalize();
            for (int neighbor_2 : n2c[n]) {
                if (neighbor_1 == neighbor_2) continue;
                auto normal2 = inf::Cell(m, neighbor_2).faceAreaNormal(0);
                normal2.normalize();
                double dot = Parfait::Point<double>::dot(normal1, normal2);
                smallest_dot[n] = std::min(smallest_dot[n], dot);
            }
        }
    }
    for (auto& d : smallest_dot) {
        d = 1.0 - d;
    }
    return smallest_dot;
}
}