#pragma once
#include <parfait/TetGenWriter.h>
#include "TinfMesh.h"
#include <parfait/Point.h>
#include <vector>

namespace inf {
inline void writeTetGenPoly(std::string filename,
                            const inf::TinfMesh& mesh,
                            std::vector<Parfait::Point<double>> hole_points) {
    auto get_point = [&](int n) {
        Parfait::Point<double> p = mesh.node(n);
        return p;
    };

    auto fill_tri = [&](int t, long* tri) {
        for (int i = 0; i < 3; i++) {
            tri[i] = mesh.mesh.cells.at(inf::MeshInterface::TRI_3)[3 * t + i];
        }
    };

    auto fill_qua = [&](int q, long* qua) {
        for (int i = 0; i < 4; i++) {
            qua[i] = mesh.mesh.cells.at(inf::MeshInterface::QUAD_4)[4 * q + i];
        }
    };

    auto get_tri_tag = [&](int t) { return mesh.mesh.cell_tags.at(inf::MeshInterface::TRI_3)[t]; };
    auto get_qua_tag = [&](int t) { return mesh.mesh.cell_tags.at(inf::MeshInterface::QUAD_4)[t]; };

    auto get_hole_point = [&](int i) { return hole_points[i]; };
    int num_tris = mesh.cellCount(inf::MeshInterface::TRI_3);
    int num_quads = mesh.cellCount(inf::MeshInterface::QUAD_4);
    Parfait::TetGen::writePoly(filename,
                               get_point,
                               mesh.nodeCount(),
                               fill_tri,
                               get_tri_tag,
                               num_tris,
                               fill_qua,
                               get_qua_tag,
                               num_quads,
                               hole_points.size(),
                               get_hole_point);
}
}