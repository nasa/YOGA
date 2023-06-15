#include "GraphBuilder.h"

GraphBuilder::GraphBuilder(const NaiveMesh& mesh) : mesh(mesh) {}

void GraphBuilder::addCellEdges(std::vector<std::set<long>>& n2n, const std::vector<long>& cell) const {
    for (auto n : cell) {
        if (not mesh.doOwnNode(n)) continue;
        auto local = mesh.global_to_local_node.at(n);
        for (auto m : cell) {
            if (n == m) continue;  //-- don't add a null edge
            n2n[local].insert(m);
        }
    }
}

void GraphBuilder::addEdgesForCellType(const inf::MeshInterface::CellType& type,
                                       std::vector<std::set<long>>& n2n) const {
    std::vector<long> cell(inf::MeshInterface::cellTypeLength(type));
    for (int c = 0; c < mesh.cellCount(type); c++) {
        mesh.cell(type, c, cell.data());
        addCellEdges(n2n, cell);
    }
}

std::vector<std::vector<long>> GraphBuilder::flattenGraph(const std::vector<std::set<long>>& n2n) const {
    std::vector<std::vector<long>> n2n_out;
    n2n_out.reserve(n2n.size());
    for (const auto& n : n2n) {
        n2n_out.emplace_back(n.begin(), n.end());
    }
    return n2n_out;
}

std::vector<std::vector<long>> GraphBuilder::build() {
    std::vector<std::set<long>> n2n(mesh.ownedCount());

    auto cell_types = mesh.cellTypes();
    for (auto type : cell_types) {
        addEdgesForCellType(type, n2n);
    }

    return flattenGraph(n2n);
}
