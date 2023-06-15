#include "HangingEdgeRemesher.h"
#include <set>
#include <vector>
#include <t-infinity/Cell.h>
#include <t-infinity/FaceNeighbors.h>
#include <t-infinity/MeshSanityChecker.h>
#include <t-infinity/MeshHelpers.h>
#include <parfait/ToString.h>
#include <t-infinity/MeshShuffle.h>

namespace inf {
namespace HangingEdge {
    enum { NOT_FOUND = -1 };

    class Remesher {
      public:
        Remesher(MessagePasser mp, std::shared_ptr<inf::MeshInterface>&& mesh)
            : mp(mp), builder(mp, std::move(mesh)) {}

        void remesh() {
            remeshLocal();
            bool need_to_shuffle =
                not cells_that_need_to_be_fixed_but_are_too_close_to_a_part_boundary.empty();
            need_to_shuffle = mp.ParallelOr(need_to_shuffle);
            while (need_to_shuffle) {
                mp_rootprint("Some hanging edge regions are too close to partition boundaries.\n");
                mp_rootprint("Reshuffling mesh. This may take a while.\n");
                auto mesh = builder.mesh;
                builder.mesh.reset();
                mesh = MeshShuffle::claimCellStencils(
                    mp, *mesh, cells_that_need_to_be_fixed_but_are_too_close_to_a_part_boundary);
                builder = inf::experimental::MeshBuilder(mp, std::move(mesh));
                cells_that_need_to_be_fixed_but_are_too_close_to_a_part_boundary.clear();
                remeshLocal();
                need_to_shuffle =
                    not cells_that_need_to_be_fixed_but_are_too_close_to_a_part_boundary.empty();
                need_to_shuffle = mp.ParallelOr(need_to_shuffle);
            }
        }

        void remeshLocal() {
            for (int iter = 0; iter < 1000; iter++) {
                if (iter == 999) {
                    PARFAIT_WARNING(
                        "More than 1000 hanging edges were found.\nThis indicates a serious "
                        "problem with the "
                        "grid.\nThe "
                        "grid is unusable.\n Exiting\n\n");
                }
                bool should_check_for_another = remeshNextQuadCell();
                if (not should_check_for_another) break;
            }
            builder.sync();
            mp_rootprint("Everyone has completed round of local remeshing\n");
        }

      public:
        MessagePasser mp;
        inf::experimental::MeshBuilder builder;
        std::set<int> cells_that_need_to_be_fixed_but_are_too_close_to_a_part_boundary;

        bool remeshNextQuadCell() {
            using CellType = MeshInterface::CellType;

            auto n2c = NodeToCell::build(*builder.mesh);
            auto face_neighbors = FaceNeighbors::build(*builder.mesh, n2c);

            int next_quad_cell = -1;
            while (true) {
                next_quad_cell =
                    findNextCellToDelete(*builder.mesh, n2c, face_neighbors, next_quad_cell + 1);
                if (next_quad_cell == NOT_FOUND) {
                    return false;
                }
                long next_quad_cell_gid = builder.mesh->globalCellId(next_quad_cell);
                printf("Rank %d Found hanging edge cell %ld\n", mp.Rank(), next_quad_cell_gid);
                auto entry = builder.mesh->cellIdToTypeAndLocalId(next_quad_cell);
                if (not builder.isCellModifiable({entry.first, entry.second})) {
                    printf("skipping hanging cell at partition boundary\n");
                    cells_that_need_to_be_fixed_but_are_too_close_to_a_part_boundary.insert(
                        next_quad_cell);
                } else {
                    break;
                }
            }

            CellType type;
            int index;
            std::tie(type, index) = builder.mesh->cell_id_to_type_and_local_id.at(next_quad_cell);
            std::vector<int> cell_nodes(inf::MeshInterface::cellTypeLength(type));
            builder.mesh->cell(next_quad_cell, cell_nodes.data());

            auto cells_near_cavity = getNodeNeighborsOfCell(*builder.mesh, next_quad_cell, n2c);
            auto cavity_faces =
                getFacesPointingToCellOrMissing(*builder.mesh,
                                                builder.mesh->cell_id_to_type_and_local_id,
                                                cells_near_cavity,
                                                face_neighbors,
                                                std::set<int>(cell_nodes.begin(), cell_nodes.end()),
                                                next_quad_cell);
            builder.deleteCell(type, index);

            builder.steinerCavity(cavity_faces);
            builder.rebuildLocal();

            return true;
        }
    };

    std::set<int> getCellNodes(const TinfMeshData& mesh,
                               inf::MeshInterface::CellType type,
                               int type_index) {
        int length = MeshInterface::cellTypeLength(type);
        std::set<int> cell_nodes;
        for (int i = 0; i < length; i++) {
            int n = mesh.cells.at(type).at(length * type_index + i);
            cell_nodes.insert(n);
        }
        return cell_nodes;
    }

    std::vector<int> getFaceNodesPointingTowardsVolumeNeighbor(int face_number,
                                                               MeshInterface::CellType type,
                                                               const std::vector<int>& cell_nodes) {
        auto face_nodes = Cell::faceNodes(face_number, type, cell_nodes, 3);
        if (MeshInterface::is2DCellType(type)) {
            // surface faces point out of the computational domain, not into the neighboring volume
            // cell, but we need them wound just like a volume face would be.
            std::reverse(face_nodes.begin(), face_nodes.end());
        }
        return face_nodes;
    }

    bool allNodesInSet(const std::vector<int>& face_nodes, const std::set<int>& node_set) {
        for (auto n : face_nodes) {
            if (node_set.count(n) == 0) return false;
        }
        return true;
    }

    std::vector<std::vector<int>> getFacesPointingToCellOrMissing(
        const MeshInterface& mesh,
        const std::vector<std::pair<MeshInterface::CellType, int>>& c2type_index_pair,
        const std::vector<int>& cell_ids_near_cavity,
        const std::vector<std::vector<int>>& face_neighbors,
        const std::set<int>& target_cell_nodes,
        int target_cell_id) {
        using CellType = MeshInterface::CellType;

        std::vector<std::vector<int>> cavity_faces;

        for (auto& cell : cell_ids_near_cavity) {
            CellType type;
            int type_index;
            std::tie(type, type_index) = c2type_index_pair.at(cell);
            std::vector<int> cell_nodes;
            mesh.cell(cell, cell_nodes);
            for (int face_number = 0; face_number < int(face_neighbors[cell].size());
                 face_number++) {
                int face_neighbor = face_neighbors[cell][face_number];
                auto face_nodes =
                    getFaceNodesPointingTowardsVolumeNeighbor(face_number, type, cell_nodes);
                if ((face_neighbor == target_cell_id or
                     face_neighbor == FaceNeighbors::NEIGHBOR_OFF_RANK) and
                    allNodesInSet(face_nodes, target_cell_nodes)) {
                    cavity_faces.push_back(face_nodes);
                }
            }
        }
        return cavity_faces;
    }
    std::vector<int> getNodeNeighborsOfCell(const MeshInterface& mesh,
                                            int cell_id,
                                            const std::vector<std::vector<int>>& n2c) {
        std::vector<int> cell_nodes;
        mesh.cell(cell_id, cell_nodes);
        std::set<int> neighbors;
        for (auto n : cell_nodes) {
            for (auto neighbor : n2c[n]) {
                if (neighbor != cell_id) neighbors.insert(neighbor);
            }
        }

        return std::vector<int>(neighbors.begin(), neighbors.end());
    }
    int findNextCellToDelete(const MeshInterface& mesh,
                             const std::vector<std::vector<int>>& n2c,
                             const std::vector<std::vector<int>>& face_neighbors,
                             int first_cell_to_check) {
        for (int cell_id = first_cell_to_check; cell_id < mesh.cellCount(); cell_id++) {
            if (mesh.ownedCell(cell_id)) {
                for (unsigned int face_number = 0; face_number < face_neighbors.at(cell_id).size();
                     face_number++) {
                    auto neighbor = face_neighbors.at(cell_id).at(face_number);
                    if (neighbor < 0) {
                        auto hanging_edge =
                            MeshSanityChecker::checkForTwoTriFacesOnQuad(mesh, cell_id, n2c);
                        if (hanging_edge.isHangingEdge()) {
                            //                            printf("Found hanging edge in cell %d,
                            //                            type %s\n",
                            //                                   cell_id,
                            //                                   inf::MeshInterface::cellTypeString(mesh.cellType(cell_id)).c_str());
                            return cell_id;
                        }
                    }
                }
            }
        }
        return NOT_FOUND;
    }

    std::shared_ptr<TinfMesh> remesh(MessagePasser mp,
                                     std::shared_ptr<MeshInterface>&& input_mesh) {
        Remesher remesher(mp, std::move(input_mesh));

        remesher.remesh();
        return remesher.builder.mesh;
    }

}
}