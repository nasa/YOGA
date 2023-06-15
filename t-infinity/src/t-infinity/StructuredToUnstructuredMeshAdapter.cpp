#include <parfait/SetTools.h>
#include <parfait/VectorTools.h>
#include <parfait/CGNSElements.h>
#include "StructuredToUnstructuredMeshAdapter.h"
#include "StructuredBlockConnectivity.h"
#include "VectorFieldAdapter.h"
#include "TinfMeshAppender.h"
#include "MeshShuffle.h"
#include "Cell.h"
#include "MeshBuilder.h"
#include "MeshHelpers.h"
#include "FaceNeighbors.h"
#include "parfait/Checkpoint.h"
#include <Tracer.h>

using namespace inf;

enum CellStatus { UNSET = -1 };
int StructuredMeshSurfaceCellTag::encodeTag(int block_id, BlockSide side) {
    return (block_id + 1) * 10 + (side + 1);
}
int StructuredMeshSurfaceCellTag::getBlockId(int tag) { return tag / 10 - 1; }
BlockSide StructuredMeshSurfaceCellTag::getBlockSide(int tag) {
    return static_cast<BlockSide>(tag % 10 - 1);
}

void appendSurfaceElements(TinfMesh& mesh,
                           long& cell_gid,
                           const StructuredMeshGlobalIds& global_cells,
                           const std::map<long, int>& cell_g2l,
                           const std::vector<std::vector<int>>& face_neighbors) {
    TRACER_PROFILE_FUNCTION
    auto& mesh_data = mesh.mesh;
    for (const auto& p : global_cells.ids) {
        int block_id = p.first;
        std::vector<long> nodes(4);
        std::vector<int> node_owner(4);
        for (auto side : AllBlockSides) {
            int tag = StructuredMeshSurfaceCellTag::encodeTag(block_id, side);
            std::array<int, 3> start = {0, 0, 0};
            std::array<int, 3> end = p.second.dimensions();
            auto face_side = BlockSideToHexOrdering[side];
            auto axis = sideToAxis(side);
            if (not isMaxFace(side)) end[axis] = 1;
            start[axis] = end[axis] - 1;
            loopMDRange(start, end, [&](int i, int j, int k) {
                long gid = p.second(i, j, k);
                if (global_cells.owner.getOwningBlock(gid) != block_id) return;
                int cell_id = cell_g2l.at(gid);
                if (face_neighbors.at(cell_id).at(face_side) != FaceNeighbors::NEIGHBOR_OFF_RANK)
                    return;
                Cell c(mesh, cell_id);
                for (int node_id : c.faceNodes(face_side)) {
                    mesh_data.cells[inf::MeshInterface::QUAD_4].push_back(node_id);
                }
                mesh_data.cell_tags[inf::MeshInterface::QUAD_4].push_back(tag);
                mesh_data.global_cell_id[inf::MeshInterface::QUAD_4].push_back(cell_gid++);
                mesh_data.cell_owner[inf::MeshInterface::QUAD_4].push_back(mesh.partitionId());
            });
        }
    }
}

void addSurfaceElements(const MessagePasser& mp,
                        TinfMesh& mesh,
                        const StructuredMeshGlobalIds& global_cells) {
    TRACER_PROFILE_FUNCTION
    auto face_neighbors = FaceNeighbors::build(mesh);
    long cell_gid = globalCellCount(mp, mesh, MeshInterface::HEXA_8);
    auto cell_g2l = GlobalToLocal::buildCell(mesh);

    for (int rank = 0; rank < mp.NumberOfProcesses(); ++rank) {
        if (mp.Rank() == rank)
            appendSurfaceElements(mesh, cell_gid, global_cells, cell_g2l, face_neighbors);
        mp.Broadcast(cell_gid, rank);
    }

    mesh.rebuild();
}

std::unordered_map<long, int> buildMeshGlobalToLocal(const StructuredMeshGlobalIds& global_nodes) {
    TRACER_PROFILE_FUNCTION
    std::set<long> node_gids;
    for (const auto& ids : global_nodes.ids) {
        const auto& global_node_ids = ids.second;
        loopMDRange({0, 0, 0}, global_node_ids.dimensions(), [&](int i, int j, int k) {
            node_gids.insert(global_node_ids(i, j, k));
        });
    }
    std::unordered_map<long, int> node_g2l;
    int local_id = 0;
    for (auto& gid : node_gids) node_g2l[gid] = local_id++;
    return node_g2l;
}

std::shared_ptr<StructuredTinfMesh> inf::convertStructuredMeshToUnstructuredMesh(
    const MessagePasser& mp,
    const StructuredMesh& mesh,
    const StructuredMeshGlobalIds& global_nodes,
    const StructuredMeshGlobalIds& global_cells) {
    TRACER_PROFILE_FUNCTION
    auto node_g2l = buildMeshGlobalToLocal(global_nodes);

    TinfMeshData mesh_data;

    Tracer::begin("Set node data");
    mesh_data.points.resize(node_g2l.size());
    mesh_data.global_node_id.resize(node_g2l.size());
    mesh_data.node_owner.resize(node_g2l.size());
    Tracer::begin("Set global node ids");
    for (const auto& g2l : node_g2l) mesh_data.global_node_id[g2l.second] = g2l.first;
    Tracer::end("Set global node ids");
    Tracer::begin("Set node owners");
    for (int node_id = 0; node_id < (int)mesh_data.global_node_id.size(); ++node_id)
        mesh_data.node_owner[node_id] =
            global_nodes.owner.getOwningRank(mesh_data.global_node_id[node_id]);
    Tracer::end("Set node owners");
    Tracer::end("Set node data");

    Tracer::begin("Set hex cell data");
    auto& hex_cells = mesh_data.cells[MeshInterface::HEXA_8];
    auto& hex_cell_gids = mesh_data.global_cell_id[MeshInterface::HEXA_8];
    auto& hex_cell_owners = mesh_data.cell_owner[MeshInterface::HEXA_8];
    for (int block_id : mesh.blockIds()) {
        Tracer::begin("Converting block: " + std::to_string(block_id));
        const auto& global_cell_ids = global_cells.ids.at(block_id);
        const auto& global_node_ids = global_nodes.ids.at(block_id);
        const auto& block = mesh.getBlock(block_id);
        loopMDRange({0, 0, 0}, global_cell_ids.dimensions(), [&](int i, int j, int k) {
            for (int corner = 0; corner < 8; ++corner) {
                auto offset = HexNodeOrdering[corner];
                auto node_gid = global_node_ids(i + offset[0], j + offset[1], k + offset[2]);
                auto lid = node_g2l.at(node_gid);
                hex_cells.push_back(lid);
            }
            auto cell_gid = global_cell_ids(i, j, k);
            hex_cell_gids.push_back(cell_gid);
            hex_cell_owners.push_back(global_cells.owner.getOwningRank(cell_gid));
        });
        loopMDRange({0, 0, 0}, global_node_ids.dimensions(), [&](int i, int j, int k) {
            auto node_id = node_g2l.at(global_node_ids(i, j, k));
            mesh_data.points[node_id] = block.point(i, j, k);
        });
        Tracer::end("Converting block: " + std::to_string(block_id));
    }
    mesh_data.cell_tags[MeshInterface::HEXA_8] =
        std::vector<int>(mesh_data.global_cell_id[MeshInterface::HEXA_8].size(), 0);
    Tracer::end("Set hex cell data");

    Tracer::begin("building volume mesh");
    auto volume_mesh = std::make_shared<TinfMesh>(mesh_data, mp.Rank());
    Tracer::end("building volume mesh");

    if (mp.NumberOfProcesses() > 1) {
        volume_mesh = MeshShuffle::extendNodeSupport(mp, *volume_mesh);
        volume_mesh = MeshShuffle::extendCellSupport(mp, *volume_mesh);
    }
    addSurfaceElements(mp, *volume_mesh, global_cells);
    if (mp.NumberOfProcesses() > 1) volume_mesh = MeshShuffle::extendCellSupport(mp, *volume_mesh);
    return std::make_shared<StructuredTinfMesh>(
        volume_mesh->mesh, mp.Rank(), global_nodes.ids, global_cells.ids);
}

std::shared_ptr<StructuredTinfMesh> inf::convertStructuredMeshToUnstructuredMesh(
    const MessagePasser& mp, const StructuredMesh& mesh) {
    TRACER_PROFILE_FUNCTION
    StructuredBlockDimensions node_dimensions, cell_dimensions;
    for (int block_id : mesh.blockIds()) {
        node_dimensions[block_id] = mesh.getBlock(block_id).nodeDimensions();
        for (int axis = 0; axis < 3; ++axis)
            cell_dimensions[block_id][axis] = node_dimensions[block_id][axis] - 1;
    }
    auto global_nodes = assignUniqueGlobalNodeIds(mp, mesh);
    auto global_cells = assignStructuredMeshGlobalCellIds(mp, cell_dimensions);
    return convertStructuredMeshToUnstructuredMesh(mp, mesh, global_nodes, global_cells);
}
std::shared_ptr<StructuredTinfMesh> inf::convertStructuredMeshToUnstructuredMesh(
    const MessagePasser& mp, const StructuredMesh& mesh, const MeshConnectivity& connectivity) {
    TRACER_PROFILE_FUNCTION
    StructuredBlockDimensions node_dimensions, cell_dimensions;
    for (int block_id : mesh.blockIds()) {
        node_dimensions[block_id] = mesh.getBlock(block_id).nodeDimensions();
        for (int axis = 0; axis < 3; ++axis)
            cell_dimensions[block_id][axis] = node_dimensions[block_id][axis] - 1;
    }
    auto global_nodes = assignStructuredMeshGlobalNodeIds(mp, connectivity, node_dimensions);
    auto global_cells = assignStructuredMeshGlobalCellIds(mp, cell_dimensions);
    return convertStructuredMeshToUnstructuredMesh(mp, mesh, global_nodes, global_cells);
}

std::shared_ptr<FieldInterface> inf::buildFieldInterfaceFromStructuredField(
    const inf::StructuredField& structured_field) {
    std::vector<double> full_field;
    for (int block_id : structured_field.blockIds()) {
        const auto& field = structured_field.getBlockField(block_id);
        auto d = field.dimensions();
        for (int i = 0; i < d[0]; ++i) {
            for (int j = 0; j < d[1]; ++j) {
                for (int k = 0; k < d[2]; ++k) {
                    full_field.push_back(field.value(i, j, k));
                }
            }
        }
    }
    return std::make_shared<VectorFieldAdapter>(
        structured_field.name(), structured_field.association(), 1, full_field);
}

int getMatchingFaceId(const std::vector<int>& cells, int cell_to_match) {
    for (int f = 0; f < 6; ++f) {
        if (cells[f] == cell_to_match) return f;
    }
    PARFAIT_THROW("Could not find face neighbor for ghost cell attached to cell: " +
                  std::to_string(cell_to_match));
}

int getFaceNeighborOppositeCellId(const std::vector<std::vector<int>>& face_neighbors,
                                  int my_cell_id,
                                  int neighbor_cell_id) {
    int neighbor_face_id = getMatchingFaceId(face_neighbors[neighbor_cell_id], my_cell_id);
    auto neighbor_side = HexOrderingToBlockSize[neighbor_face_id];
    auto neighbor_opposite_side = getOppositeSide(neighbor_side);
    auto neighbor_opposite_face_id = BlockSideToHexOrdering[neighbor_opposite_side];
    return face_neighbors[neighbor_cell_id][neighbor_opposite_face_id];
}

std::set<long> buildGlobalIdsOnBlock(const Vector3D<long>& global_ids) {
    std::set<long> gids_on_block = {global_ids.vec.begin(), global_ids.vec.end()};
    gids_on_block.erase(-1);
    return gids_on_block;
}
std::vector<int> findNodesInCommon(const std::vector<int>& my_nodes,
                                   const std::vector<int>& their_nodes) {
    auto in_common =
        Parfait::SetTools::Intersection(std::set<int>{my_nodes.begin(), my_nodes.end()},
                                        std::set<int>{their_nodes.begin(), their_nodes.end()});
    return {in_common.begin(), in_common.end()};
}

int getSharedEdgeId(const std::vector<int>& nodes) {
    for (int i = 0; i < Parfait::CGNS::Hex::numberOfEdges(); ++i) {
        const auto& edge_nodes = Parfait::CGNS::Hex::edge_to_node[i];
        if (nodes[0] == edge_nodes[0] and nodes[1] == edge_nodes[1]) return i;
        if (nodes[1] == edge_nodes[0] and nodes[0] == edge_nodes[1]) return i;
    }
    PARFAIT_THROW("Could not find edge");
}

int getNodePositionInCell(const std::vector<int>& cell_nodes, int node_id) {
    for (int i = 0; i < (int)cell_nodes.size(); ++i) {
        if (node_id == cell_nodes[i]) return i;
    }
    PARFAIT_THROW("Could not find node index");
}

void setGlobalCellIdsOnEdgesAndCorners(StructuredBlockGlobalIds& block_global_ids,
                                       const MessagePasser& mp,
                                       const StructuredMeshGlobalIds& global_cells,
                                       const MeshInterface& mesh) {
    TRACER_PROFILE_FUNCTION
    auto c2c = CellToCell::build(mesh);
    auto g2l = GlobalToLocal::buildCell(mesh);
    for (auto& p : block_global_ids) {
        int block_id = p.first;
        auto& gids = p.second;
        std::array<int, 3> start{};
        std::array<int, 3> end = global_cells.ids.at(block_id).dimensions();
        for (int direction = 0; direction < 3; ++direction) {
            int n_ghost = (gids.dimensions()[direction] - end[direction]) / 2;
            start[direction] += n_ghost;
            end[direction] += n_ghost;
        }
        auto globals_that_have_already_been_set = buildGlobalIdsOnBlock(gids);

        auto setGID = [&](int i, int j, int k, long gid) {
            if (gids(i, j, k) == UNSET) gids(i, j, k) = gid;
        };

        std::vector<int> me, neighbor;
        loopMDRange(start, end, [&](int i, int j, int k) {
            int local_cell_id = g2l.at(gids(i, j, k));
            const auto& neighbor_local_cell_ids = c2c[local_cell_id];

            for (int neib : neighbor_local_cell_ids) {
                auto neighbor_gid = mesh.globalCellId(neib);
                if (globals_that_have_already_been_set.count(neighbor_gid) == 0) {
                    mesh.cell(local_cell_id, me);
                    mesh.cell(neib, neighbor);
                    auto nodes_in_common = findNodesInCommon(me, neighbor);
                    if (2 == nodes_in_common.size()) {  // Edge Neighbor
                        std::vector<int> shared_nodes = {
                            getNodePositionInCell(me, nodes_in_common[0]),
                            getNodePositionInCell(me, nodes_in_common[1])};
                        int shared_edge_id = getSharedEdgeId(shared_nodes);
                        auto offset = HexEdgeNeighborOffset[shared_edge_id];
                        setGID(i + offset[0], j + offset[1], k + offset[2], neighbor_gid);
                    } else if (1 == nodes_in_common.size()) {  // Corner Neighbor
                        int shared_node = getNodePositionInCell(me, nodes_in_common.front());
                        auto offset = HexCornerNeighborOffset[shared_node];
                        setGID(i + offset[0], j + offset[1], k + offset[2], neighbor_gid);
                    }
                }
            }
        });
    }
}

StructuredBlockGlobalIds inf::getGlobalCellIdsWithGhostCells(
    const MessagePasser& mp,
    const StructuredMeshGlobalIds& global_cells,
    const MeshInterface& mesh,
    int num_ghost_cells_per_block_side) {
    TRACER_PROFILE_FUNCTION
    PARFAIT_ASSERT(num_ghost_cells_per_block_side <= 2,
                   "Only support up to two levels of ghost cells")
    if (num_ghost_cells_per_block_side == 0) return global_cells.ids;
    auto face_neighbors = FaceNeighbors::build(mesh);
    auto g2l = GlobalToLocal::buildCell(mesh);
    StructuredBlockGlobalIds global_cell_ids_with_ghosts;
    for (const auto& p : global_cells.ids) {
        auto block_global_ids = padMDVector(p.second, num_ghost_cells_per_block_side, long(UNSET));

        // 2) Set any cells not already set via cell-to-cell connections
        std::array<int, 3> start{};
        std::array<int, 3> end = p.second.dimensions();
        for (int direction = 0; direction < 3; ++direction) {
            start[direction] += num_ghost_cells_per_block_side;
            end[direction] += num_ghost_cells_per_block_side;
        }
        loopMDRange(start, end, [&](int i, int j, int k) {
            auto gid = block_global_ids(i, j, k);
            if (gid == UNSET) return;
            int local_cell_id = g2l.at(gid);
            for (auto side : AllBlockSides) {
                auto ijk = shiftIJK({i, j, k}, side, 1);
                auto& neighbor_gid = block_global_ids(ijk[0], ijk[1], ijk[2]);

                if (neighbor_gid != UNSET) continue;  // Skip setting cells that were previous set

                auto face_index = BlockSideToHexOrdering[side];
                int neighbor_local_id = face_neighbors[local_cell_id][face_index];
                neighbor_gid = mesh.globalCellId(neighbor_local_id);
                if (num_ghost_cells_per_block_side == 2 and
                    MeshInterface::is3DCellType(mesh.cellType(neighbor_local_id))) {
                    int neighbor_second_level_local_cell_id = getFaceNeighborOppositeCellId(
                        face_neighbors, local_cell_id, neighbor_local_id);
                    ijk = shiftIJK({i, j, k}, side, 2);
                    block_global_ids(ijk[0], ijk[1], ijk[2]) =
                        mesh.globalCellId(neighbor_second_level_local_cell_id);
                }
            }
        });
        global_cell_ids_with_ghosts[p.first] = block_global_ids;
    }
    setGlobalCellIdsOnEdgesAndCorners(global_cell_ids_with_ghosts, mp, global_cells, mesh);
    return global_cell_ids_with_ghosts;
}

StructuredBlockGlobalIds inf::getGlobalNodeIdsWithGhostNodes(
    const MessagePasser& mp,
    const StructuredMeshGlobalIds& global_cells,
    const MeshInterface& mesh) {
    TRACER_PROFILE_FUNCTION
    auto g2l = GlobalToLocal::buildCell(mesh);
    auto n2n = NodeToNode::build(mesh);
    auto total_hexes = globalCellCount(mp, mesh, MeshInterface::HEXA_8);
    auto isVolumeCellGID = [&](long gid) { return gid >= 0 and gid < total_hexes; };
    StructuredBlockGlobalIds global_nodes;
    for (const auto& p : global_cells.ids) {
        auto cellIsOwnedByThisBlock = [&](long gid) {
            return p.first == global_cells.owner.getOwningBlock(gid);
        };
        auto dimensions = p.second.dimensions();
        for (int& d : dimensions) d += 1;
        auto& block_nodes = global_nodes[p.first];
        block_nodes = Vector3D<long>(dimensions, UNSET);
        std::vector<int> cell_local_nodes(8);
        std::vector<int> neighbor_local_nodes(8);
        auto setOwnedCellNodes = [&](int i, int j, int k) {
            auto gid = p.second(i, j, k);
            if (not isVolumeCellGID(gid)) return;
            if (not cellIsOwnedByThisBlock(gid)) return;

            // Set global node IDs for nodes in owned cell
            mesh.cell(g2l.at(gid), cell_local_nodes);
            for (int corner = 0; corner < 8; ++corner) {
                auto offset = HexNodeOrdering[corner];
                block_nodes(i + offset[0], j + offset[1], k + offset[2]) =
                    mesh.globalNodeId(cell_local_nodes[corner]);
            }

            // Set global node IDs for nodes in neighbor cells on each side
            for (auto side : AllBlockSides) {
                auto neighbor_ijk = shiftIJK({i, j, k}, side, 1);
                auto neighbor_gid = p.second(neighbor_ijk[0], neighbor_ijk[1], neighbor_ijk[2]);
                if (not isVolumeCellGID(neighbor_gid)) continue;
                if (cellIsOwnedByThisBlock(neighbor_gid)) continue;

                mesh.cell(g2l.at(neighbor_gid), neighbor_local_nodes);
                auto nodes_in_common = findNodesInCommon(cell_local_nodes, neighbor_local_nodes);
                for (int n : nodes_in_common) {
                    int my_corner = getNodePositionInCell(cell_local_nodes, n);
                    auto ijk = shiftIJK(HexNodeOrdering[my_corner], side, 1);
                    auto& opposite_node_gid = block_nodes(i + ijk[0], j + ijk[1], k + ijk[2]);
                    if (opposite_node_gid != UNSET) continue;

                    int their_corner = getNodePositionInCell(neighbor_local_nodes, n);
                    for (const auto& edge : Parfait::CGNS::Hex::edge_to_node) {
                        using namespace Parfait::VectorTools;
                        if (isIn(nodes_in_common, neighbor_local_nodes[edge.front()]) and
                            isIn(nodes_in_common, neighbor_local_nodes[edge.back()]))
                            continue;
                        if (edge.front() == their_corner) {
                            int opposite_node_id = neighbor_local_nodes[edge.back()];
                            opposite_node_gid = mesh.globalNodeId(opposite_node_id);
                        } else if (edge.back() == their_corner) {
                            int opposite_node_id = neighbor_local_nodes[edge.front()];
                            opposite_node_gid = mesh.globalNodeId(opposite_node_id);
                        }
                    }
                }
            }
        };
        loopMDRange({0, 0, 0}, p.second.dimensions(), setOwnedCellNodes);
    }
    return global_nodes;
}
