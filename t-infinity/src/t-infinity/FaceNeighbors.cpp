#include "FaceNeighbors.h"
#include "parfait/ToString.h"
#include <parfait/CGNSElements.h>
#include <parfait/Throw.h>
#include <parfait/VectorTools.h>
#include <Tracer.h>

int cellTypeFaceCount2D(inf::MeshInterface::CellType type) {
    using namespace inf;
    switch (type) {
        case MeshInterface::NODE:
            return 0;
        case MeshInterface::BAR_2:
            return 1;
        case MeshInterface::TRI_3:
            return 3;
        case MeshInterface::QUAD_4:
            return 4;
        default:
            PARFAIT_THROW("face count of type " + MeshInterface::cellTypeString(type) +
                          " not supported");
    }
}
int cellTypeFaceCount(inf::MeshInterface::CellType type) {
    using namespace inf;
    switch (type) {
        case MeshInterface::NODE:
            return 0;
        case MeshInterface::BAR_2:
            return 0;
        case MeshInterface::TRI_3:
        case MeshInterface::QUAD_4:
            return 1;
        case MeshInterface::TETRA_4:
            return 4;
        case MeshInterface::PYRA_5:
        case MeshInterface::PENTA_6:
            return 5;
        case MeshInterface::HEXA_8:
            return 6;
        default:
            PARFAIT_THROW("face count of type " + MeshInterface::cellTypeString(type) +
                          " not supported");
    }
}

std::vector<std::vector<int>> inf::FaceNeighbors::build(
    const inf::MeshInterface& mesh, const std::vector<std::vector<int>>& node_to_cell) {
    TRACER_PROFILE_SCOPE("FaceNeighbors::build")
    std::vector<std::vector<int>> neighbors(mesh.cellCount());
    std::vector<int> cell;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        auto type = mesh.cellType(cell_id);
        mesh.cell(cell_id, cell);
        auto candidates = getNodeNeighborsOfCell(node_to_cell, cell, cell_id);
        neighbors[cell_id] = getFaceNeighbors(mesh, type, cell, candidates);
    }
    return neighbors;
}
std::vector<std::vector<int>> inf::FaceNeighbors::buildInMode(const inf::MeshInterface& mesh,
                                                              int twod_or_threed) {
    TRACER_PROFILE_SCOPE("FaceNeighbors::buildInMode")
    if (twod_or_threed == 3) return build(mesh);
    if (twod_or_threed == 2) return buildAssuming2DMesh(mesh);
    PARFAIT_THROW("Unknown FaceNeighbors mode " + std::to_string(twod_or_threed));
}
std::vector<std::vector<int>> inf::FaceNeighbors::buildOwnedOnly(
    const inf::MeshInterface& mesh, const std::vector<std::vector<int>>& node_to_cell) {
    TRACER_PROFILE_SCOPE("FaceNeighbors::buildOwnedOnly")
    std::vector<std::vector<int>> neighbors(mesh.cellCount());
    std::vector<int> cell;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        if (mesh.partitionId() == mesh.cellOwner(cell_id)) {
            auto type = mesh.cellType(cell_id);
            mesh.cell(cell_id, cell);
            auto candidates = getNodeNeighborsOfCell(node_to_cell, cell, cell_id);
            neighbors[cell_id] = getFaceNeighbors(mesh, type, cell, candidates);
        }
    }
    return neighbors;
}
std::vector<std::vector<int>> inf::FaceNeighbors::buildOwnedOnly(const inf::MeshInterface& mesh) {
    TRACER_PROFILE_SCOPE("FaceNeighbors::buildOwnedOnly")
    return buildOwnedOnly(mesh, inf::NodeToCell::build(mesh));
}
std::vector<std::vector<int>> inf::FaceNeighbors::build(const inf::MeshInterface& mesh) {
    TRACER_PROFILE_SCOPE("FaceNeighbors::build")
    return build(mesh, inf::NodeToCell::build(mesh));
}
std::vector<std::vector<int>> inf::FaceNeighbors::buildOnlyForCellsWithDimensionality(
    const inf::MeshInterface& mesh, int dimensionality) {
    TRACER_PROFILE_SCOPE("FaceNeighbors::buildOnlyForCellsWithDimensionality")
    auto node_to_cell = inf::NodeToCell::build(mesh);
    std::vector<std::vector<int>> neighbors(mesh.cellCount());
    std::vector<int> cell;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        if (mesh.cellDimensionality(cell_id) == dimensionality) {
            mesh.cell(cell_id, cell);
            auto candidates = getNodeNeighborsOfCell(node_to_cell, cell, cell_id);
            neighbors[cell_id] =
                dimensionality == 1
                    ? getFaceNeighbors2D(mesh, mesh.cellType(cell_id), cell, candidates)
                    : getFaceNeighbors(mesh, mesh.cellType(cell_id), cell, candidates);
        }
    }
    return neighbors;
}
std::vector<std::vector<int>> inf::FaceNeighbors::buildAssuming2DMesh(
    const inf::MeshInterface& mesh) {
    TRACER_PROFILE_SCOPE("FaceNeighbors::buildAssuming2DMesh")
    auto node_to_cell = inf::NodeToCell::build(mesh);
    std::vector<std::vector<int>> neighbors(mesh.cellCount());
    std::vector<int> cell;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        mesh.cell(cell_id, cell);
        auto candidates = getNodeNeighborsOfCell(node_to_cell, cell, cell_id);
        neighbors[cell_id] = getFaceNeighbors2D(mesh, mesh.cellType(cell_id), cell, candidates);
    }
    return neighbors;
}
std::vector<int> inf::FaceNeighbors::getFaceNeighbors(const inf::MeshInterface& mesh,
                                                      const inf::MeshInterface::CellType& type,
                                                      const std::vector<int>& cell,
                                                      const std::vector<int>& candidates) {
    std::vector<int> cell_neighbors;
    int num_faces = cellTypeFaceCount(type);
    std::vector<int> face;
    for (int f = 0; f < num_faces; f++) {
        getFace(type, cell, f, face);
        int face_neighbor = findFaceNeighbor(mesh, candidates, face);
        cell_neighbors.push_back(face_neighbor);
    }
    return cell_neighbors;
}
std::vector<int> inf::FaceNeighbors::getFaceNeighbors2D(const inf::MeshInterface& mesh,
                                                        const inf::MeshInterface::CellType& type,
                                                        const std::vector<int>& cell,
                                                        const std::vector<int>& candidates) {
    std::vector<int> cell_neighbors;
    int num_faces = cellTypeFaceCount2D(type);
    std::vector<int> face;
    for (int f = 0; f < num_faces; f++) {
        getFace2D(type, cell, f, face);
        if (type == inf::MeshInterface::QUAD_4) {
            for (int c : candidates) {
                std::vector<int> neighbor_nodes;
                mesh.cell(c, neighbor_nodes);
            }
        }
        int face_neighbor = findFaceNeighbor(mesh, candidates, face);
        cell_neighbors.push_back(face_neighbor);
    }
    return cell_neighbors;
}
std::vector<int> inf::FaceNeighbors::getNodeNeighborsOfCell(
    const std::vector<std::vector<int>>& node_to_cell, const std::vector<int>& cell, int cell_id) {
    // This is convoluted for better performance, The original implementation used std::set, which
    // was ~4x slower.
    int neighbor_count = 0;
    for (int node : cell) {
        neighbor_count += node_to_cell.at(node).size();
    }

    std::vector<int> neighbors;
    neighbors.reserve(neighbor_count);
    for (int node : cell) {
        for (int c : node_to_cell[node]) {
            if (c != cell_id and not Parfait::VectorTools::isIn(neighbors, c)) {
                neighbors.push_back(c);
            }
        }
    }
    neighbors.shrink_to_fit();
    return neighbors;
}
bool inf::FaceNeighbors::cellContainsFace(const std::vector<int>& cell,
                                          const std::vector<int>& face) {
    for (auto& id : face)
        if (std::find(cell.begin(), cell.end(), id) == cell.end()) return false;
    return true;
}
std::vector<std::array<long, 2>> inf::FaceNeighbors::flattenToGlobals(
    const inf::MeshInterface& mesh, const std::vector<std::vector<int>>& face_neighbors) {
    std::vector<std::array<long, 2>> faces;
    for (int cell_id = 0; cell_id < int(face_neighbors.size()); cell_id++) {
        auto global = mesh.globalCellId(cell_id);
        for (auto neighbor : face_neighbors[cell_id]) {
            if (neighbor != FaceNeighbors::NEIGHBOR_OFF_RANK) {
                auto neighbor_global = mesh.globalCellId(neighbor);
                if (global < neighbor_global) {
                    faces.push_back({global, neighbor_global});
                }
            }
        }
    }
    return faces;
}
