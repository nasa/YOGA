#include "AddMissingFaces.h"
#include "FaceNeighbors.h"
#include "Cell.h"
#include "MeshHelpers.h"
#include "MeshBuilder.h"

using inf::MeshInterface;

void inf::assignOwnedSurfaceCellGlobalIds(const MessagePasser& mp, inf::TinfMesh& mesh) {
    auto global_id = std::max<long>(globalCellCount(mp, mesh) - 1, 0);
    auto setCellId = [&](MeshInterface::CellType cell_type) {
        for (int i = 0; i < mesh.cellCount(cell_type); ++i) {
            auto& gid = mesh.mesh.global_cell_id[cell_type][i];
            if (gid == -1 and mesh.mesh.cell_owner[cell_type][i] == mesh.partitionId())
                gid = global_id++;
        }
    };
    for (int rank = 0; rank < mp.NumberOfProcesses(); ++rank) {
        if (rank == mesh.partitionId()) {
            setCellId(MeshInterface::TRI_3);
            setCellId(MeshInterface::QUAD_4);
        }
        mp.Broadcast(global_id, rank);
    }
}
std::shared_ptr<inf::TinfMesh> inf::addMissingFaces(
    MessagePasser mp,
    const inf::MeshInterface& m,
    const std::vector<std::vector<int>>& face_neighbors,
    int new_tag) {
    inf::experimental::MeshBuilder builder(mp, m);
    for (int i = 0; i < m.cellCount(); i++) {
        if (m.cellOwner(i) != m.partitionId()) continue;
        int n = face_neighbors[i].size();
        for (int j = 0; j < n; j++) {
            if (face_neighbors[i][j] == FaceNeighbors::NEIGHBOR_OFF_RANK) {
                auto cell = Cell(*builder.mesh, i);
                auto face = cell.faceNodes(j);
                if (face.size() == 3) {
                    builder.addCell(MeshInterface::TRI_3, face, new_tag);
                } else {
                    builder.addCell(MeshInterface::QUAD_4, face, new_tag);
                }
            }
        }
    }
    builder.sync();
    return builder.mesh;
}
std::shared_ptr<inf::TinfMesh> inf::addMissingFaces(MessagePasser mp,
                                                    const inf::MeshInterface& m,
                                                    int new_tag) {
    auto face_neighbors = FaceNeighbors::build(m);
    return addMissingFaces(mp, m, face_neighbors, new_tag);
}
