#include "MeshExploder.h"
#include "MeshHelpers.h"
#include "Cell.h"

void copyNodeData(MessagePasser mp,
                  inf::TinfMesh& mesh,
                  std::vector<int> new_to_old,
                  long total_original_node_count) {
    long num_new_nodes = new_to_old.size();

    auto original_points = mesh.mesh.points;
    mesh.mesh.points.resize(num_new_nodes);
    for (unsigned int node = 0; node < num_new_nodes; node++) {
        int old_id = new_to_old[node];
        mesh.mesh.points[node] = original_points[old_id];
    }

    auto original_global_node_ids = mesh.mesh.global_node_id;
    mesh.mesh.global_node_id.resize(num_new_nodes);
    auto node_pro_range = mp.Gather(num_new_nodes);
    long my_gids_start_at = 0;
    for (int r = 0; r < mp.Rank() - 1; r++) {
        my_gids_start_at += node_pro_range[r];
    }
    long next_node_gid = my_gids_start_at;
    for (unsigned int node = 0; node < num_new_nodes; node++) {
        mesh.mesh.global_node_id[node] = next_node_gid++;
    }

    auto original_node_owner = mesh.mesh.node_owner;
    mesh.mesh.node_owner.resize(num_new_nodes);
    for (unsigned int node = 0; node < num_new_nodes; node++) {
        int old_id = new_to_old[node];
        mesh.mesh.node_owner[node] = original_node_owner[old_id];
    }
}

void duplicatePoints(MessagePasser mp, inf::TinfMesh& mesh, int dimension) {
    int num_new_points = 0;
    for (int cellId = 0; cellId < mesh.cellCount(); cellId++) {
        num_new_points += mesh.cellLength(cellId);
    }
    std::vector<int> new_to_old_node_id(num_new_points);
    int new_node_id = 0;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        auto cell = inf::Cell(mesh, cell_id, dimension);
        auto points = cell.points();
        auto pair = mesh.cellIdToTypeAndLocalId(cell_id);
        auto length = mesh.cellLength(cell_id);
        auto* cell_nodes = &mesh.mesh.cells[pair.first][pair.second * length];
        for (int i = 0; i < length; i++) {
            int old = cell_nodes[i];
            new_to_old_node_id[new_node_id] = old;
            cell_nodes[i] = new_node_id++;
        }
    }

    copyNodeData(mp, mesh, new_to_old_node_id, inf::globalNodeCount(mp, mesh));
}

void shrinkPoints(inf::TinfMesh& mesh, int dimension, double scale) {
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        auto cell = inf::Cell(mesh, cell_id, dimension);
        auto points = cell.points();
        auto center = cell.averageCenter();
        auto pair = mesh.cellIdToTypeAndLocalId(cell_id);
        auto length = mesh.cellLength(cell_id);
        auto* cell_nodes = &mesh.mesh.cells[pair.first][pair.second * length];
        for (int i = 0; i < length; i++) {
            auto n = cell_nodes[i];
            auto p = mesh.mesh.points[n];
            auto dp = p - center;
            dp *= scale;
            mesh.mesh.points[n] = center + dp;
        }
    }
}

std::shared_ptr<inf::TinfMesh> inf::explodeMesh(MessagePasser mp,
                                                std::shared_ptr<inf::MeshInterface> mesh_input,
                                                double scale) {
    auto mesh = std::make_shared<inf::TinfMesh>(mesh_input);
    int dimension = inf::maxCellDimensionality(mp, *mesh);
    duplicatePoints(mp, *mesh, dimension);
    shrinkPoints(*mesh, dimension, scale);

    mesh = std::make_shared<inf::TinfMesh>(mesh->mesh, mesh->partitionId());
    return mesh;
}
