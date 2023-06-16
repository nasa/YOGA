#include "SurfaceNeighbors.h"
#include <parfait/Throw.h>
#include <parfait/VectorTools.h>
#include <Tracer.h>

std::vector<std::vector<int>> SurfaceNeighbors::buildNode2VolumeCell(
    const std::shared_ptr<inf::TinfMesh>& mesh) {
    Tracer::begin(__FUNCTION__);
    std::vector<std::vector<int>> node2Cell(mesh->nodeCount());

    for (int cell_id = 0; cell_id < mesh->cellCount(); cell_id++) {
        auto cell_type = mesh->cellType(cell_id);
        if (mesh->is3DCellType(cell_type)) {
            int cell_length = mesh->cellTypeLength(cell_type);
            std::vector<int> cell(cell_length);
            mesh->cell(cell_id, cell.data());
            for (int i = 0; i < cell_length; i++) {
                int node = cell[i];
                Parfait::VectorTools::insertUnique(node2Cell[node], cell_id);
            }
        }
    }
    Tracer::end(__FUNCTION__);
    return node2Cell;
}
int SurfaceNeighbors::findNeighbor(const std::shared_ptr<inf::TinfMesh> mesh,
                                   const std::vector<std::vector<int>>& n2c,
                                   const int* cell,
                                   int cell_length) {
    auto neighbors = n2c[cell[0]];
    std::set<int> candidates(neighbors.begin(), neighbors.end());
    for (int i = 1; i < cell_length; i++) {
        auto cell_neighbors = n2c[cell[i]];
        for (auto c : neighbors) {
            if (not Parfait::VectorTools::isIn(cell_neighbors, c)) {
                candidates.erase(c);
            }
        }
    }
    if (candidates.size() != 1) {
        std::string s = "Neighbor types: ";
        for (int nbr : candidates) {
            s.append(mesh->cellTypeString(mesh->cellType(nbr)));
            s.append(" ");
        }
        s.append("\n");
        PARFAIT_THROW(
            "Found an incorrect number of volume neighbors for surface element.  "
            "Should be 1, found: " +
            std::to_string(candidates.size()) + s);
    }
    return *candidates.begin();
}
std::vector<int> SurfaceNeighbors::buildSurfaceNeighbors(
    const std::shared_ptr<inf::TinfMesh>& mesh) {
    Tracer::begin(__FUNCTION__);
    std::vector<int> neighbors;

    auto n2c = buildNode2VolumeCell(mesh);

    int num_surface = 0;
    for (int i = 0; i < mesh->cellCount(); i++) {
        auto type = mesh->cellType(i);
        if (mesh->is2DCellType(type)) num_surface++;
    }
    neighbors.resize(num_surface);

    int surface_index = 0;
    std::vector<int> cell;
    for (int cell_id = 0; cell_id < mesh->cellCount(); cell_id++) {
        auto cell_type = mesh->cellType(cell_id);
        if (mesh->is2DCellType(cell_type)) {
            int cell_length = mesh->cellTypeLength(cell_type);
            cell.resize(cell_length);
            mesh->cell(cell_id, cell.data());
            neighbors[surface_index++] = findNeighbor(mesh, n2c, cell.data(), cell_length);
        }
    }
    Tracer::end(__FUNCTION__);
    return neighbors;
}
