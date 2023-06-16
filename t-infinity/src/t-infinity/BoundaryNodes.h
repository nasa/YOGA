#pragma once
#include <parfait/SetTools.h>
#include <t-infinity/MeshInterface.h>

#include <set>

namespace inf {
namespace BoundaryNodes {
    inline std::set<int> buildSet(const inf::MeshInterface& mesh) {
        std::set<int> boundary_nodes;
        std::vector<int> cell;
        cell.reserve(4);
        for (int c = 0; c < mesh.cellCount(); c++) {
            auto type = mesh.cellType(c);
            if (mesh.is2DCellType(type)) {
                cell.resize(mesh.cellTypeLength(type));
                mesh.cell(c, cell.data());
                for (int n : cell) {
                    boundary_nodes.insert(n);
                }
            }
        }
        return boundary_nodes;
    }
}

namespace InteriorNodes {
    inline std::set<int> buildSet(const inf::MeshInterface& mesh) {
        std::set<int> interior_nodes;
        for (int n = 0; n < mesh.nodeCount(); n++) {
            interior_nodes.insert(n);
        }
        auto boundary_nodes = BoundaryNodes::buildSet(mesh);
        return Parfait::SetTools::Difference(interior_nodes, boundary_nodes);
    }
}
}