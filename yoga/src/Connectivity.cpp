#include "Connectivity.h"
#include <thread>
#include <Tracer.h>

namespace YOGA {
std::vector<std::vector<int>> Connectivity::nodeToCell(const YogaMesh& mesh) {
    std::vector<std::vector<int>> n2c(mesh.nodeCount());
    std::vector<int> cell;
    for (int i = 0; i < mesh.numberOfCells(); i++) {
        cell.resize(mesh.numberOfNodesInCell(i));
        mesh.getNodesInCell(i, cell.data());
        for (int node : cell) n2c[node].push_back(i);
    }
    return n2c;
}

std::vector<std::vector<int>> Connectivity::nodeToNode(const YogaMesh& mesh) {
    Tracer::begin("Build n2n");
    std::vector<std::set<int>> n2n(mesh.nodeCount());
    Tracer::begin("Build n2c");
    auto n2c = nodeToCell(mesh);
    Tracer::end("Build n2c");
    std::vector<int> cell;
    for (int i = 0; i < mesh.nodeCount(); i++) {
        for (int cell_id : n2c[i]) {
            cell.resize(mesh.numberOfNodesInCell(cell_id));
            mesh.getNodesInCell(cell_id, cell.data());
            for (int nbr : cell)
                if (nbr != i) n2n[i].insert(nbr);
        }
    }
    Tracer::begin("convert");
    std::vector<std::vector<int>> node_to_node(n2n.size());
    for (size_t i = 0; i < n2n.size(); i++) node_to_node[i] = std::vector<int>(n2n[i].begin(), n2n[i].end());
    Tracer::end("convert");
    Tracer::end("Build n2n");
    return node_to_node;
}
}