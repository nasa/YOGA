#pragma once
#include <array>
#include "VizualizationWriter.h"
#include "Throw.h"

namespace Parfait {
namespace tecplot {
    template <typename CellNodes>
    inline std::array<int, 8> collapseCellToBrick(int type, const CellNodes& cell_nodes) {
        using namespace Parfait::Visualization;
        std::array<int, 8> out;
        switch (type) {
            case CellType::NODE:
                out = {cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0]};
                break;
            case CellType::EDGE:
                out = {cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[1],
                       cell_nodes[1],
                       cell_nodes[1],
                       cell_nodes[1]};
                break;
            case CellType::TRI:
                out = {cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[1],
                       cell_nodes[1],
                       cell_nodes[2],
                       cell_nodes[2]};
                break;
            case CellType::QUAD:
                out = {cell_nodes[0],
                       cell_nodes[1],
                       cell_nodes[2],
                       cell_nodes[3],
                       cell_nodes[0],
                       cell_nodes[1],
                       cell_nodes[2],
                       cell_nodes[3]};
                break;
            case CellType::TET:
                out = {cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[1],
                       cell_nodes[1],
                       cell_nodes[2],
                       cell_nodes[3]};
                break;
            case CellType::PYRAMID:
                out = {cell_nodes[0],
                       cell_nodes[1],
                       cell_nodes[2],
                       cell_nodes[3],
                       cell_nodes[4],
                       cell_nodes[4],
                       cell_nodes[4],
                       cell_nodes[4]};
                break;
            case CellType::PRISM:
                out = {cell_nodes[0],
                       cell_nodes[1],
                       cell_nodes[2],
                       cell_nodes[2],
                       cell_nodes[3],
                       cell_nodes[4],
                       cell_nodes[5],
                       cell_nodes[5]};
                break;
            case CellType::HEX:
                out = {cell_nodes[0],
                       cell_nodes[1],
                       cell_nodes[2],
                       cell_nodes[3],
                       cell_nodes[4],
                       cell_nodes[5],
                       cell_nodes[6],
                       cell_nodes[7]};
                break;
            case CellType::TRI_P2:
                out = {cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[0],
                       cell_nodes[1],
                       cell_nodes[1],
                       cell_nodes[2],
                       cell_nodes[2]};
                break;
            default:
                PARFAIT_WARNING("Unknown cell type trying to convert to tecplot degenerate brick: " +
                                std::to_string(type));
        }
        return out;
    }
}
}
