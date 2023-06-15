#pragma once
#include <algorithm>
#include <memory>
#include <set>
#include <vector>
#include <array>
#include <t-infinity/MeshInterface.h>
#include <parfait/CGNSFaceExtraction.h>
#include <parfait/Throw.h>
#include "MeshConnectivity.h"
#include "parfait/CGNSElements.h"

namespace inf {
class FaceNeighbors {
  public:
    static const int NEIGHBOR_OFF_RANK = -1;

    static std::vector<std::vector<int>> buildInMode(const inf::MeshInterface& mesh,
                                                     int twod_or_threed);
    static std::vector<std::vector<int>> buildAssuming2DMesh(const inf::MeshInterface& mesh);
    static std::vector<std::vector<int>> build(const inf::MeshInterface& mesh,
                                               const std::vector<std::vector<int>>& node_to_cell);
    static std::vector<std::vector<int>> buildOwnedOnly(
        const inf::MeshInterface& mesh, const std::vector<std::vector<int>>& node_to_cell);

    static std::vector<std::vector<int>> build(const inf::MeshInterface& mesh);
    static std::vector<std::vector<int>> buildOwnedOnly(const inf::MeshInterface& mesh);
    static std::vector<std::vector<int>> buildOnlyForCellsWithDimensionality(
        const inf::MeshInterface& mesh, int dimensionality);
    static std::vector<std::array<long, 2>> flattenToGlobals(
        const inf::MeshInterface& mesh, const std::vector<std::vector<int>>& face_neighbors);

    static bool cellContainsFace(const std::vector<int>& cell, const std::vector<int>& face);

  public:
    template <typename T>
    static void getFace2D(inf::MeshInterface::CellType cell_type,
                          const std::vector<T>& cell,
                          int face_id,
                          std::vector<T>& face_nodes) {
        face_nodes.resize(2);
        switch (cell_type) {
            case inf::MeshInterface::BAR_2:
                PARFAIT_ASSERT(face_id < 1, "Bar face " + std::to_string(face_id) + " invalid")
                face_nodes = cell;
                break;
            case inf::MeshInterface::TRI_3:
                PARFAIT_ASSERT(face_id < 3, "Tri face " + std::to_string(face_id) + " invalid")
                face_nodes[0] = cell.at(Parfait::CGNS::Triangle::edge_to_node[face_id][0]);
                face_nodes[1] = cell.at(Parfait::CGNS::Triangle::edge_to_node[face_id][1]);
                break;
            case inf::MeshInterface::QUAD_4:
                PARFAIT_ASSERT(face_id < 4, "Quad face " + std::to_string(face_id) + " invalid")
                face_nodes[0] = cell.at(Parfait::CGNS::Quad::edge_to_node[face_id][0]);
                face_nodes[1] = cell.at(Parfait::CGNS::Quad::edge_to_node[face_id][1]);
                break;
            default:
                PARFAIT_THROW("Encountered unexpected face " + std::to_string(face_id) +
                              " for type " + inf::MeshInterface::cellTypeString(cell_type));
        }
    }

    static std::vector<int> getFaceNeighbors(const MeshInterface& mesh,
                                             const MeshInterface::CellType& type,
                                             const std::vector<int>& cell,
                                             const std::vector<int>& candidates);
    static std::vector<int> getFaceNeighbors2D(const MeshInterface& mesh,
                                               const MeshInterface::CellType& type,
                                               const std::vector<int>& cell,
                                               const std::vector<int>& candidates);

    template <typename Collection>
    static int findFaceNeighbor(const inf::MeshInterface& mesh,
                                const Collection& candidates,
                                const std::vector<int>& face) {
        std::vector<int> neighbor;
        for (auto neighbor_id : candidates) {
            mesh.cell(neighbor_id, neighbor);
            if (cellContainsFace(neighbor, face)) return neighbor_id;
        }
        return NEIGHBOR_OFF_RANK;
    }

    static std::vector<int> getNodeNeighborsOfCell(
        const std::vector<std::vector<int>>& node_to_cell,
        const std::vector<int>& cell,
        int cell_id);

    template <typename T>
    static void getFace(inf::MeshInterface::CellType cell_type,
                        const std::vector<T>& cell,
                        int face_id,
                        std::vector<T>& face) {
        switch (cell_type) {
            case inf::MeshInterface::TRI_3:
                face = cell;
                break;
            case inf::MeshInterface::QUAD_4:
                face = cell;
                break;
            case inf::MeshInterface::TETRA_4:
                Parfait::CGNS::getTetFace(cell, face_id, face);
                break;
            case inf::MeshInterface::PYRA_5:
                Parfait::CGNS::getPyramidFace(cell, face_id, face);
                break;
            case inf::MeshInterface::PENTA_6:
                Parfait::CGNS::getPrismFace(cell, face_id, face);
                break;
            case inf::MeshInterface::HEXA_8:
                Parfait::CGNS::getHexFace(cell, face_id, face);
                break;
            default:
                PARFAIT_THROW("Encountered unexpected face");
        }
    }
};
}
