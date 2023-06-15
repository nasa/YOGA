#pragma once
#include <functional>
#include <vector>
#include <t-infinity/MeshInterface.h>
#include <parfait/Point.h>
#include <parfait/Extent.h>
#include <parfait/ExtentBuilder.h>

namespace inf {

namespace filter_tools {
    inline std::vector<Parfait::Point<double>> getPointsInCell(const inf::MeshInterface& mesh,
                                                               int cell_id) {
        int n = mesh.cellTypeLength(mesh.cellType(cell_id));
        std::vector<Parfait::Point<double>> points(n);
        std::vector<int> cell_node_ids(n);
        mesh.cell(cell_id, cell_node_ids.data());
        for (int i = 0; i < n; i++) mesh.nodeCoordinate(cell_node_ids[i], points[i].data());
        return points;
    }

    inline std::vector<int> selectCellsByExtent(
        std::shared_ptr<inf::MeshInterface> mesh,
        std::function<bool(const Parfait::Extent<double>&)> contains) {
        std::vector<int> cell_ids;
        for (int cell_id = 0; cell_id < mesh->cellCount(); cell_id++) {
            auto points = filter_tools::getPointsInCell(*mesh.get(), cell_id);
            auto e = Parfait::ExtentBuilder::build(points);
            if (contains(e)) cell_ids.push_back(cell_id);
        }
        return cell_ids;
    }

    inline std::vector<int> selectCellsByCellId(const inf::MeshInterface& mesh,
                                                std::function<bool(int)> contains) {
        std::vector<int> cell_ids;
        for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++)
            if (contains(cell_id)) cell_ids.push_back(cell_id);
        return cell_ids;
    }
}
}