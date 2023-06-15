#pragma once
#include "MeshInterface.h"
#include "MeshConnectivity.h"
#include <parfait/Point.h>
#include <vector>

namespace inf {
inline std::vector<double> calcNodeAvgLength(const MeshInterface& mesh) {
    auto n2n = NodeToNode::build(mesh);
    std::vector<double> avg_length(mesh.nodeCount());

    for (int n = 0; n < mesh.nodeCount(); n++) {
        Parfait::Point<double> p = mesh.node(n);
        avg_length[n] = 0.0;
        for (auto neighbor : n2n[n]) {
            Parfait::Point<double> q = mesh.node(neighbor);
            double length = (p - q).magnitude();
            avg_length[n] += length;
        }
        avg_length[n] /= double(n2n[n].size());
    }

    return avg_length;
}
inline std::vector<double> calcNodeMinLength(const MeshInterface& mesh) {
    auto n2n = NodeToNode::build(mesh);
    std::vector<double> min_length(mesh.nodeCount());

    for (int n = 0; n < mesh.nodeCount(); n++) {
        Parfait::Point<double> p = mesh.node(n);
        min_length[n] = 1e200;
        for (auto neighbor : n2n[n]) {
            Parfait::Point<double> q = mesh.node(neighbor);
            double length = (p - q).magnitude();
            min_length[n] = std::min(min_length[n], length);
        }
        min_length[n] /= double(n2n[n].size());
    }

    return min_length;
}
}