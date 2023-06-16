#pragma once
#include <t-infinity/TinfMesh.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/Extract.h>
#include <t-infinity/MeshHelpers.h>
#include "parfait/Wiggle.h"
#include "GlobalToLocal.h"
#include "parfait/SyncField.h"

namespace inf {
inline double shortestDistance(const TinfMesh& mesh, int node, const std::vector<int>& neighbors) {
    double d = std::numeric_limits<double>::max();
    Parfait::Point<double> a, b;
    mesh.nodeCoordinate(node, a.data());
    for (int nbr : neighbors) {
        mesh.nodeCoordinate(nbr, b.data());
        d = std::min(d, a.distance(b));
    }
    return d;
}
inline void wiggleNodes(MessagePasser mp, TinfMesh& mesh, double percent = 1.0) {
    auto n2n = NodeToNode::build(mesh);
    Parfait::Wiggler w;
    percent /= 100.0;
    for (int node = 0; node < mesh.nodeCount(); node++) {
        if (mesh.nodeOwner(node) != mesh.partitionId()) {
            continue;
        }
        auto& neighbors = n2n[node];
        double d = shortestDistance(mesh, node, neighbors);
        w.wiggle(mesh.mesh.points[node], percent * d);
    }

    auto node_sync_pattern = buildNodeSyncPattern(mp, mesh);
    auto g2l = GlobalToLocal::buildNode(mesh);
    auto points = mesh.mesh.points;
    Parfait::syncVector(mp, points, g2l, node_sync_pattern);
    mesh.mesh.points = points;
}
}