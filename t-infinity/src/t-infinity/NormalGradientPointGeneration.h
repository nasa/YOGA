#pragma once
#include <unordered_set>
#include <unordered_map>
#include <parfait/Point.h>
#include <parfait/Normals.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/Cell.h>
#include <parfait/Throw.h>

namespace inf {
class WallNormalPoints {
  public:
    inline static std::unordered_map<int, Parfait::Point<double>> generateWallNormalPointsFromCells(
        const inf::MeshInterface& mesh, const std::set<int>& wall_tags, double height) {
        auto cells_on_surface = cellsInTags(mesh, wall_tags);

        std::unordered_map<int, Parfait::Point<double>> points_for_surface_cells;
        for (auto c : cells_on_surface) {
            inf::Cell cell(mesh, c);
            auto center = cell.averageCenter();
            auto normal = -1.0 * cell.faceAreaNormal(0);
            normal.normalize();
            points_for_surface_cells[c] = center + height * normal;
        }
        return points_for_surface_cells;
    }

    inline static std::unordered_map<int, Parfait::Point<double>> generateWallNormalPointsFromNodes(
        const inf::MeshInterface& mesh, const std::set<int>& wall_tags, double height) {
        auto nodes_on_surface = nodesInTags(mesh, wall_tags);
        auto n2c = buildNodeToSurfaceCellInTags(mesh, nodes_on_surface, wall_tags);

        std::unordered_map<int, Parfait::Point<double>> points_for_surface_nodes;

        for (const auto& pair : n2c) {
            auto node_id = pair.first;
            const auto& cells = pair.second;
            std::vector<Parfait::Point<double>> cell_normals;
            for (auto c : cells) {
                auto cell = inf::Cell(mesh, c);
                cell_normals.push_back(-1.0 * cell.faceAreaNormal(0));
            }

            auto normal = Parfait::Normals::goodNormal(cell_normals);
            Parfait::Point<double> p = mesh.node(node_id);
            points_for_surface_nodes[node_id] = p + height * normal;
        }
        return points_for_surface_nodes;
    }

    inline static std::set<int> cellsInTags(const inf::MeshInterface& mesh,
                                            const std::set<int>& wall_tags) {
        std::set<int> cells_in_tags;

        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.is2DCell(c)) {
                auto tag = mesh.cellTag(c);
                if (wall_tags.count(tag)) cells_in_tags.insert(c);
            }
        }
        return cells_in_tags;
    }

    inline static std::unordered_set<int> nodesInTags(const inf::MeshInterface& mesh,
                                                      const std::set<int>& wall_tags) {
        std::unordered_set<int> nodes_in_tags;
        std::vector<int> cell_nodes;
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.is2DCell(c)) {
                auto tag = mesh.cellTag(c);
                if (wall_tags.count(tag)) {
                    mesh.cell(c, cell_nodes);
                    for (auto n : cell_nodes) nodes_in_tags.insert(n);
                }
            }
        }
        return nodes_in_tags;
    }

    inline static std::unordered_map<int, std::unordered_set<int>> buildNodeToSurfaceCellInTags(
        const inf::MeshInterface& mesh,
        const std::unordered_set<int>& nodes_i_want_neighbors_of,
        const std::set<int>& wall_tags) {
        std::unordered_map<int, std::unordered_set<int>> n2c;

        std::vector<int> cell_nodes;
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.is2DCell(c)) {
                auto tag = mesh.cellTag(c);
                if (wall_tags.count(tag)) {
                    mesh.cell(c, cell_nodes);
                    for (auto n : cell_nodes) {
                        if (nodes_i_want_neighbors_of.count(n)) {
                            n2c[n].insert(c);
                        }
                    }
                }
            }
        }
        return n2c;
    }
};
}
