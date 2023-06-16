#pragma once
#include <memory>
#include <vector>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/MeshBuilder.h>

namespace inf {
namespace HangingEdge {

    std::shared_ptr<inf::TinfMesh> remesh(MessagePasser mp,
                                          std::shared_ptr<inf::MeshInterface>&& mesh);

    int findNextCellToDelete(const inf::MeshInterface& mesh,
                             const std::vector<std::vector<int>>& n2c,
                             const std::vector<std::vector<int>>& face_neighbors,
                             int next_cell_to_check = 0);

    std::vector<int> getNodeNeighborsOfCell(const inf::MeshInterface& mesh,
                                            int cell_id,
                                            const std::vector<std::vector<int>>& n2c);

    std::vector<int> getFaceNodesPointingTowardsVolumeNeighbor(int face_number,
                                                               inf::MeshInterface::CellType type,
                                                               const std::vector<int>& cell_nodes);

    std::vector<std::vector<int>> getFacesPointingToCellOrMissing(
        const inf::MeshInterface& mesh,
        const std::vector<std::pair<inf::MeshInterface::CellType, int>>& c2type_index_pair,
        const std::vector<int>& cell_ids_near_cavity,
        const std::vector<std::vector<int>>& face_neighbors,
        const std::set<int>& nodes_in_target_cell,
        int target_cell_id);

    std::set<int> getCellNodes(const inf::TinfMeshData& mesh,
                               inf::MeshInterface::CellType type,
                               int type_index);

}
}
