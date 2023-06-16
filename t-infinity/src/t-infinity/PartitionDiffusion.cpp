#include "PartitionDiffusion.h"
#include "Extract.h"
#include "MeshShuffle.h"
#include "TinfMesh.h"
#include "AddHalo.h"
#include "FaceNeighbors.h"
#include "FieldTools.h"
#include "MeshHelpers.h"

std::vector<int> inf::PartitionDiffusion::reassignCellOwners(
    const inf::MeshInterface& mesh, const std::vector<std::vector<int>>& neighbors) {
    std::vector<int> new_cell_owners = inf::extractCellOwners(mesh);

    auto most_common_owner = [&](int c) {
        std::map<int, int> owner_to_count;
        for (auto neighbor : neighbors[c]) {
            int owner = new_cell_owners[neighbor];
            owner_to_count[owner]++;
        }
        int most_common = mesh.cellOwner(c);
        int count = 1;
        for (auto& pair : owner_to_count) {
            int owner = pair.first;
            int owner_count = pair.second;
            if (owner_count > count) {
                most_common = owner;
                count = owner_count;
            }
            if (owner_count == count) {
                most_common = std::max(most_common, owner);
            }
        }
        return most_common;
    };

    for (int c = 0; c < mesh.cellCount(); c++) {
        int my_rank = mesh.partitionId();
        if (mesh.cellOwner(c) == my_rank) {
            new_cell_owners[c] = most_common_owner(c);
        }
    }
    return new_cell_owners;
}
std::vector<std::vector<int>> inf::PartitionDiffusion::addLayer(
    const std::vector<std::vector<int>>& graph) {
    int num_rows = int(graph.size());
    std::vector<std::vector<int>> extended(graph.size());
    for (int c = 0; c < num_rows; c++) {
        std::set<int> neighbors;
        for (auto& first_neighbor : graph[c]) {
            if (first_neighbor < 0) continue;
            for (auto& second_neighbor : graph[first_neighbor]) {
                if (second_neighbor >= 0 and second_neighbor < num_rows)
                    neighbors.insert(second_neighbor);
            }
        }
        extended[c] = std::vector<int>(neighbors.begin(), neighbors.end());
    }
    return extended;
}
std::shared_ptr<inf::TinfMesh> inf::PartitionDiffusion::cellBasedDiffusion(
    MessagePasser mp, std::shared_ptr<inf::MeshInterface> mesh, int num_sweeps) {
    auto surface_cell_dimensionality = maxCellDimensionality(mp, *mesh) - 1;

    for (int i = 0; i < num_sweeps; i++) {
        mesh = inf::addHaloNodes(mp, *mesh);
        auto node_to_cell = inf::NodeToCell::build(*mesh);
        auto cell_to_cell = inf::CellToCell::build(*mesh, node_to_cell);
        auto face_neighbors = inf::FaceNeighbors::build(*mesh, node_to_cell);
        auto face_neighbors_of_face_neighbors = addLayer(face_neighbors);

        auto new_owners =
            inf::PartitionDiffusion::reassignCellOwners(*mesh, face_neighbors_of_face_neighbors);
        FieldTools::setSurfaceCellToVolumeNeighborValue(
            *mesh, face_neighbors, surface_cell_dimensionality, new_owners);

        int num_moved = 0;
        for (int c = 0; c < mesh->cellCount(); c++) {
            if (mesh->cellOwner(c) != new_owners[c]) num_moved++;
        }
        num_moved = mp.ParallelSum(num_moved);
        mp_rootprint("num cells moved: %d\n", num_moved);
        if (num_moved == 0) break;
        mesh = inf::MeshShuffle::shuffleCells(mp, *mesh, new_owners);
    }
    return std::make_shared<TinfMesh>(mesh);
}
