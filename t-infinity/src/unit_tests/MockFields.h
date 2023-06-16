#pragma once
#include <vector>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/FieldInterface.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/Cell.h>

namespace inf  {
namespace test {
    template <typename Field>
    std::vector<double> fillFieldAtCells(const inf::MeshInterface& mesh, Field fieldCallback) {
        std::vector<double> cell_field(mesh.cellCount());
        for (int c = 0; c < mesh.cellCount(); c++) {
            auto cell = inf::Cell(mesh, c);
            auto p = cell.averageCenter();
            cell_field[c] = fieldCallback(p[0], p[1], p[2]);
        }
        return cell_field;
    }

    template <typename Field>
    std::vector<double> fillFieldAtNodes(const inf::MeshInterface& mesh, Field fieldCallback) {
        std::vector<double> node_field(mesh.nodeCount());
        for (int n = 0; n < mesh.nodeCount(); n++) {
            auto p = mesh.node(n);
            node_field[n] = fieldCallback(p[0], p[1], p[2]);
        }
        return node_field;
    }
}

inline std::shared_ptr<inf::FieldInterface> createCellField(const std::vector<double>& f,
                                                                 std::string name = "cell_field") {
    return std::make_shared<inf::VectorFieldAdapter>(name, inf::FieldAttributes::Cell(), 1, f);
}
inline std::shared_ptr<inf::FieldInterface> createNodeField(const std::vector<double>& f,
                                                                 std::string name = "node_field") {
    return std::make_shared<inf::VectorFieldAdapter>(name, inf::FieldAttributes::Node(), 1, f);
}
}
