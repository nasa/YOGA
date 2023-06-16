#pragma once
#include "TinfMesh.h"
#include <parfait/Throw.h>

namespace inf {
namespace SurfaceReconstruction {
    void curveMidEdgeNodesInTri6(inf::TinfMesh& p2_mesh,
                                 const std::vector<Parfait::Point<double>>& node_normals) {
        // This elevation occurs redundantly.
        // Each mid edge node will be processed twice, once for each neighboring cell.
        // Since the node normals are single valued,
        // both cells will calculate the same mid edge location.
        // If we start using multi-valued normals to handle sharp discontinuities revisit this.
        for (int c = 0; c < p2_mesh.cellCount(); c++) {
            auto type = p2_mesh.cellType(c);
            if (inf::MeshInterface::CellType::TRI_6 == type) {
                std::vector<int> cell_nodes = p2_mesh.cell(c);
                std::array<Parfait::Point<double>, 3> p = {p2_mesh.mesh.points[cell_nodes[0]],
                                                           p2_mesh.mesh.points[cell_nodes[1]],
                                                           p2_mesh.mesh.points[cell_nodes[2]]};
                std::array<Parfait::Point<double>, 3> n = {node_normals[cell_nodes[0]],
                                                           node_normals[cell_nodes[1]],
                                                           node_normals[cell_nodes[2]]};
                for (int e = 0; e < 3; e++) {
                    int a = e;
                    int b = (e + 1) % 3;
                    auto p1 = p[a];
                    auto p2 = p[b];
                    auto n1 = n[a];
                    auto n2 = n[b];
                    auto w1 = (p2 - p1).dot(n1);
                    auto w2 = (p1 - p2).dot(n2);
                    auto q = 1.0 / 2.0 * (p1 + p2 - 0.25 * (w1 * n1 + w2 * n2));
                    p2_mesh.mesh.points[cell_nodes[e + 3]] = q;
                }
            } else if (inf::MeshInterface::cellTypeDimensionality(type) == 2) {
                PARFAIT_WARNING("Surface element type " + inf::MeshInterface::cellTypeString(type) +
                                " encountered while curving an elevated P2 mesh.  \nOnly TRI_6 "
                                "elements are supported.  \nAll other mid-edge nodes will not be "
                                "elevated and will remain on the P1 surface");
            }
        }
    }
}
}