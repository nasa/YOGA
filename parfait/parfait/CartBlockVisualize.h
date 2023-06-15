#pragma once
#include "CartBlock.h"
#include "VTKWriter.h"

namespace Parfait {
inline void visualize(Parfait::CartBlock block,
                      const std::vector<double>& cell_data,
                      std::string filename = "cart-block") {
    auto getPoint = [&](int node_id, double* p) { block.getNode(node_id, p); };
    auto getVTKCellType = [](long cell_id) { return vtk::Writer::CellType::HEX; };
    auto getCellNodes = [&](int cell_id, int* cell_nodes) {
        auto nodes = block.getNodesInCell(cell_id);
        for (size_t i = 0; i < nodes.size(); i++) {
            cell_nodes[i] = nodes[i];
        }
    };
    auto getCellField = [&](long cell_id) { return cell_data[cell_id]; };
    Parfait::vtk::Writer writer(
        filename, block.numberOfNodes(), block.numberOfCells(), getPoint, getVTKCellType, getCellNodes);
    writer.addCellField("field", getCellField);
    writer.visualize();
}
inline void visualize(Parfait::CartBlock block, std::string filename = "cart-block") {
    std::vector<double> field(block.numberOfCells(), 0);
    visualize(block, field, filename);
}
}
