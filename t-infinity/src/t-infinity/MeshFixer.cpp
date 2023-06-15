#include "MeshFixer.h"
#include <tuple>
#include <algorithm>

std::shared_ptr<inf::TinfMesh> inf::MeshFixer::fixAll(
    std::shared_ptr<inf::MeshInterface> input_mesh) {
    auto mesh = std::make_shared<inf::TinfMesh>(input_mesh);

    int attempted = 0;
    int fixed = 0;
    for (int cell_id = 0; cell_id < mesh->cellCount(); cell_id++) {
        auto cell = inf::Cell(*mesh.get(), cell_id);
        if (not inf::MeshInterface::is3DCellType(cell.type())) continue;
        if (not inf::MeshSanityChecker::areCellFacesValid(cell)) {
            attempted++;
            bool is_now_valid;
            std::vector<int> cell_nodes;
            std::tie(is_now_valid, cell_nodes) = tryToUnwindCell(cell);
            if (is_now_valid) {
                fixed++;
                auto type = mesh->cellType(cell_id);
                auto length = mesh->cellTypeLength(type);
                auto beginning = &mesh->mesh.cells[type][length * cell_id];
                std::copy(cell_nodes.begin(), cell_nodes.end(), beginning);
            }
        }
    }

    printf("Attempted to rewind %d cells.  %d cells were successfully wound.\n", attempted, fixed);
    return mesh;
}
std::tuple<bool, std::vector<int>> inf::MeshFixer::tryToUnwindCell(inf::Cell cell) {
    bool valid = inf::MeshSanityChecker::areCellFacesValid(cell);
    if (valid) {
        return {valid, cell.nodes()};
    }
    std::vector<int> permutation(cell.nodes().size());
    for (size_t i = 0; i < permutation.size(); i++) {
        permutation[i] = i;
    }

    auto initial = permutation;
    auto original_cell_points = cell.points();
    auto original_cell_nodes = cell.nodes();

    std::vector<Parfait::Point<double>> permuted_cell_points(original_cell_points.size());
    std::vector<int> permuted_cell_nodes(original_cell_nodes.size());
    do {
        std::next_permutation(permutation.begin(), permutation.end());
        for (size_t i = 0; i < permuted_cell_points.size(); i++) {
            permuted_cell_points[i] = original_cell_points[permutation[i]];
            permuted_cell_nodes[i] = original_cell_nodes[permutation[i]];
        }

        if (initial == permutation) {
            break;
        }

        cell = inf::Cell(cell.type(), permuted_cell_nodes, permuted_cell_points);
        valid = inf::MeshSanityChecker::areCellFacesValid(cell);
    } while (not valid);
    return {valid, permuted_cell_nodes};
}
