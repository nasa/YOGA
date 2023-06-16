#pragma once
#include <set>
#include <vector>
#include <map>
#include <parfait/Point.h>
#include "MeshInterface.h"
#include "Extract.h"

namespace inf {
namespace LayerInsertion {

    inline inf::MeshInterface::CellType prismaticType(inf::MeshInterface::CellType surface_type) {
        switch (surface_type) {
            case inf::MeshInterface::TRI_3:
                return inf::MeshInterface::PENTA_6;
            case inf::MeshInterface::QUAD_4:
                return inf::MeshInterface::HEXA_8;
            default:
                PARFAIT_THROW("Unknown surface element type " +
                              inf::MeshInterface::cellTypeString(surface_type));
        }
    }

    inline void insertLayerForTags(inf::TinfMesh& mesh, const std::set<int>& tags) {
        int next_node_id = mesh.nodeCount();
        int next_cell_id = mesh.cellCount();

        std::vector<int> surface_cell_nodes;
        std::vector<int> scratch;
        std::vector<int> prismatic_cell_nodes;
        std::map<int, int> old_to_new_node_id;

        auto getPoint = [&](int node_id) {
            Parfait::Point<double> p;
            mesh.nodeCoordinate(node_id, p.data());
            return p;
        };

        auto getNodeId = [&](int node_id) {
            if (old_to_new_node_id.count(node_id) == 0) {
                old_to_new_node_id[node_id] = next_node_id;
                mesh.mesh.points.push_back(getPoint(node_id));
                mesh.mesh.global_node_id.push_back(next_node_id);
                mesh.mesh.node_owner.push_back(mesh.partitionId());
                next_node_id++;
            }
            return old_to_new_node_id[node_id];
        };

        auto rewindCell = [&](std::vector<int>& cell_nodes) {
            scratch = cell_nodes;
            int surface_length = int(cell_nodes.size() / 2);
            for (int i = 0; i < surface_length; i++) {
                int j = surface_length - i - 1;
                cell_nodes[(j + 1) % surface_length] = scratch[i];
                cell_nodes[(j + 1) % surface_length + surface_length] = scratch[i + surface_length];
            }
        };

        for (auto& pair : mesh.mesh.cells) {
            auto surface_type = pair.first;
            if (inf::MeshInterface::is2DCellType(surface_type)) {
                auto prismatic_type = prismaticType(surface_type);
                int surface_length = inf::MeshInterface::cellTypeLength(surface_type);
                int prismatic_length = inf::MeshInterface::cellTypeLength(prismatic_type);
                surface_cell_nodes.resize(surface_length);
                prismatic_cell_nodes.resize(prismatic_length);
                for (unsigned long c_index = 0; c_index < pair.second.size() / surface_length;
                     c_index++) {
                    auto tag = mesh.mesh.cell_tags[surface_type][c_index];
                    if (tags.count(tag) != 1) continue;
                    for (int i = 0; i < surface_length; i++) {
                        int top_node = pair.second[surface_length * c_index + i];
                        int bot_node = getNodeId(top_node);
                        pair.second[surface_length * c_index + i] = bot_node;
                        prismatic_cell_nodes[i] = bot_node;
                        prismatic_cell_nodes[i + surface_length] = top_node;
                    }
                    rewindCell(prismatic_cell_nodes);
                    for (int i = 0; i < prismatic_length; i++) {
                        mesh.mesh.cells[prismatic_type].push_back(prismatic_cell_nodes[i]);
                    }
                    mesh.mesh.cell_owner[prismatic_type].push_back(mesh.partitionId());
                    mesh.mesh.global_cell_id[prismatic_type].push_back(next_cell_id++);
                    mesh.mesh.cell_tags[prismatic_type].push_back(0);
                }
            }
        }
        mesh.rebuild();
    }

    inline void insertLayer(inf::TinfMesh& mesh) {
        auto all_tags = inf::extractTagsWithDimensionality(mesh, 2);
        insertLayerForTags(mesh, all_tags);
    }

}
}
