#include "SurfaceEdgeNeighbors.h"
#include "SurfaceMesh.h"
bool cellContainsEdge(const std::vector<int>& cell, const std::array<int, 2>& edge) {
    for (auto& id : edge)
        if (std::find(cell.begin(), cell.end(), id) == cell.end()) return false;
    return true;
}

template <typename Collection>
static int findEdgeNeighbor(const inf::MeshInterface& mesh,
                            const Collection& candidates,
                            const std::array<int, 2>& edge) {
    std::vector<int> neighbor;
    for (auto neighbor_id : candidates) {
        mesh.cell(neighbor_id, neighbor);
        if (cellContainsEdge(neighbor, edge)) return neighbor_id;
    }
    return inf::FaceNeighbors::NEIGHBOR_OFF_RANK;
}

int inf::SurfaceEdgeNeighbors::numberOfEdges(const inf::MeshInterface::CellType type) {
    int num_edges = -1;
    if (type == inf::MeshInterface::TRI_3) {
        num_edges = 3;
    } else if (type == inf::MeshInterface::QUAD_4) {
        num_edges = 4;
    }
    PARFAIT_ASSERT(num_edges != -1, "Only Tri and quad elements supported");

    return num_edges;
}

std::vector<int> getEdgeNeighbors(const inf::MeshInterface& mesh,
                                  const inf::MeshInterface::CellType& type,
                                  const std::vector<int>& cell,
                                  const std::vector<int>& candidates) {
    std::vector<int> cell_neighbors;
    PARFAIT_ASSERT(mesh.is2DCellType(type),
                   "Edge Neighbors in this way is the analog of face neighbors in 3D.\nVolume "
                   "elements not allowed");
    std::array<int, 2> edge;

    auto num_edges = inf::SurfaceEdgeNeighbors::numberOfEdges(type);

    for (int e = 0; e < num_edges; e++) {
        edge[0] = cell[e];
        edge[1] = cell[(e + 1) % num_edges];
        auto edge_neighbor = findEdgeNeighbor(mesh, candidates, edge);
        cell_neighbors.push_back(edge_neighbor);
    }
    return cell_neighbors;
}

std::vector<std::vector<int>> inf::SurfaceEdgeNeighbors::buildSurfaceEdgeNeighbors(
    const inf::MeshInterface& mesh) {
    auto n2c = inf::NodeToCell::buildSurfaceOnly(mesh);
    std::vector<std::vector<int>> neighbors(mesh.cellCount());
    std::vector<int> cell_nodes;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        auto type = mesh.cellType(cell_id);
        if (inf::MeshInterface::is2DCellType(type)) {
            mesh.cell(cell_id, cell_nodes);
            auto candidates = inf::FaceNeighbors::getNodeNeighborsOfCell(n2c, cell_nodes, cell_id);
            neighbors[cell_id] = getEdgeNeighbors(mesh, type, cell_nodes, candidates);
        }
    }
    return neighbors;
}
bool inf::SurfaceEdgeNeighbors::edgeHasNode(const std::array<int, 2>& edge, int node) {
    if (edge[0] == node or edge[1] == node) return true;
    return false;
}
int inf::SurfaceEdgeNeighbors::getOtherNode(const std::array<int, 2>& edge, int node) {
    if (edge[0] == node) return edge[1];
    if (edge[1] == node) return edge[0];
    PARFAIT_THROW("Node not in edge");
}
