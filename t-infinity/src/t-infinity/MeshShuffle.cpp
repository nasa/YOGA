#include "MeshShuffle.h"
#include <parfait/RecursiveBisection.h>
#include "SetNodeOwners.h"
#include <t-infinity/Cell.h>
#include <t-infinity/Extract.h>
#include <t-infinity/Shortcuts.h>
#include <parfait/VectorTools.h>
#include "FieldTools.h"
#include "FaceNeighbors.h"
#include "MeshHelpers.h"
#include <Tracer.h>
using namespace inf;

std::map<int, MessagePasser::Message> packFragments(
    const std::map<int, TinfMeshAppender>& fragments) {
    std::map<int, MessagePasser::Message> messages;
    for (auto& pair : fragments) {
        auto& frag = pair.second.getData();
        frag.pack(messages[pair.first]);
    }
    return messages;
}

std::map<int, TinfMeshAppender> queueFragmentsToSendBasedOnCells(
    MessagePasser mp, const MeshInterface& mesh, const std::vector<int>& new_cell_owners) {
    TRACER_PROFILE_FUNCTION
    auto c2c = inf::CellToCell::build(mesh);

    // --- find cells we need to send to each rank
    std::map<int, std::set<int>> cells_to_send_to_rank;
    for (int cell = 0; cell < mesh.cellCount(); cell++) {
        if (mesh.partitionId() == mesh.cellOwner(cell)) {
            int new_owner = new_cell_owners[cell];
            PARFAIT_ASSERT(new_owner >= 0 and new_owner < mp.NumberOfProcesses(),
                           "New node owner outside range: " + std::to_string(new_owner));
            cells_to_send_to_rank[new_owner].insert(cell);
            for (auto c : c2c[cell]) {
                cells_to_send_to_rank[new_owner].insert(c);
            }
        }
    }

    std::map<int, TinfMeshAppender> fragments;
    for (auto& pair : cells_to_send_to_rank) {
        int target = pair.first;
        auto& cell_ids = pair.second;
        for (auto c : cell_ids) {
            TinfMeshCell cell(mesh, c);
            cell.owner = new_cell_owners[c];
            for (int i = 0; i < int(cell.nodes.size()); i++) {
                cell.node_owner[i] = -1;
            }
            fragments[target].addCell(cell);
        }
    }

    return fragments;
}

std::map<int, TinfMeshAppender> queueFragmentsToSendBasedOnNodes(
    MessagePasser mp,
    const MeshInterface& mesh,
    const std::map<long, int>& g2l_node,
    const std::vector<int>& new_node_owners) {
    TRACER_PROFILE_FUNCTION
    auto n2c = inf::NodeToCell::build(mesh);

    // --- find cells we need to send to each rank
    std::map<int, std::set<int>> cells_to_send_to_rank;
    for (int node = 0; node < mesh.nodeCount(); node++) {
        if (mesh.partitionId() == mesh.nodeOwner(node)) {
            int new_owner = new_node_owners[node];
            PARFAIT_ASSERT(new_owner >= 0 and new_owner < mp.NumberOfProcesses(),
                           "New node owner outside range: " + std::to_string(new_owner));
            for (auto c : n2c[node]) {
                cells_to_send_to_rank[new_owner].insert(c);
            }
        }
    }

    std::map<int, TinfMeshAppender> fragments;
    for (auto& pair : cells_to_send_to_rank) {
        int target = pair.first;
        auto& cell_ids = pair.second;
        for (auto c : cell_ids) {
            TinfMeshCell cell(mesh, c);
            cell.owner = -1;
            for (int i = 0; i < int(cell.nodes.size()); i++) {
                long global = cell.nodes[i];
                int local = g2l_node.at(global);
                cell.node_owner[i] = new_node_owners.at(local);
            }
            fragments[target].addCell(cell);
        }
    }

    return fragments;
}

std::map<int, TinfMeshAppender> queueFragmentsToAddNodeStencilSupport(
    MessagePasser mp, const MeshInterface& mesh, const std::map<long, int>& g2l_node) {
    TRACER_PROFILE_FUNCTION
    auto n2c = inf::NodeToCell::build(mesh);

    // --- find cells we need to send to each rank
    std::map<int, std::set<int>> cells_to_send_to_rank;
    for (int node = 0; node < mesh.nodeCount(); node++) {
        int new_owner = mesh.nodeOwner(node);
        for (auto c : n2c[node]) {
            cells_to_send_to_rank[new_owner].insert(c);
        }
    }

    std::map<int, TinfMeshAppender> fragments;
    for (auto& pair : cells_to_send_to_rank) {
        int target = pair.first;
        auto& cell_ids = pair.second;
        for (auto c : cell_ids) {
            TinfMeshCell cell(mesh, c);
            fragments[target].addCell(cell);
        }
    }

    return fragments;
}

std::map<int, TinfMeshAppender> ensureCellSupport(MessagePasser mp,
                                                  const MeshInterface& mesh,
                                                  const std::map<long, int>& g2l_node) {
    TRACER_PROFILE_FUNCTION
    auto n2c = inf::NodeToCell::build(mesh);

    // --- find cells we need to send to each rank
    std::map<int, std::set<int>> cells_to_send_to_rank;
    for (int node = 0; node < mesh.nodeCount(); node++) {
        for (auto c : n2c[node]) {
            for (auto d : n2c[node]) {
                cells_to_send_to_rank[mesh.cellOwner(c)].insert(d);
            }
        }
    }

    std::map<int, TinfMeshAppender> fragments;
    for (auto& pair : cells_to_send_to_rank) {
        int target = pair.first;
        auto& cell_ids = pair.second;
        for (auto c : cell_ids) {
            TinfMeshCell cell(mesh, c);
            fragments[target].addCell(cell);
        }
    }

    return fragments;
}

void throwIfCannot_nodes(MessagePasser mp,
                         const MeshInterface& mesh,
                         const std::vector<int>& new_node_owners) {
    std::string error = "Cannot shuffle mesh based on nodes: ";

    if (int(new_node_owners.size()) != mesh.nodeCount()) {
        PARFAIT_THROW(error + "New owner size mismatch");
    }

    for (auto owner : new_node_owners) {
        if (owner < 0 or owner >= mp.NumberOfProcesses()) {
            PARFAIT_THROW(error + "Target owner (" + std::to_string(owner) +
                          ")is outsize MPI rank range");
        }
    }
}

void inferNewCellOwners(inf::TinfMesh& mesh, const std::map<long, int>& g2l_node) {
    TRACER_PROFILE_FUNCTION
    std::vector<int> cell_nodes;
    for (int c = 0; c < mesh.cellCount(); c++) {
        cell_nodes.resize(mesh.cellLength(c));
        mesh.cell(c, cell_nodes.data());
        long minimum_global_id = std::numeric_limits<long>::max();
        for (int n : cell_nodes) {
            long gid = mesh.globalNodeId(n);
            minimum_global_id = std::min(gid, minimum_global_id);
        }
        int local = g2l_node.at(minimum_global_id);
        int owner = mesh.nodeOwner(local);
        inf::MeshInterface::CellType cell_type;
        int type_index;
        std::tie(cell_type, type_index) = mesh.cellIdToTypeAndLocalId(c);
        PARFAIT_ASSERT(owner >= 0, "Cell owner must be posistive was: " + std::to_string(owner));
        mesh.mesh.cell_owner[cell_type][type_index] = owner;
    }
}

std::shared_ptr<inf::TinfMesh> MeshShuffle::shuffleCells(MessagePasser mp,
                                                         const MeshInterface& mesh,
                                                         std::vector<int> new_cell_owners) {
    TRACER_PROFILE_FUNCTION
    Parfait::SyncPattern cell_sync_pattern = buildCellSyncPattern(mp, mesh);

    auto g2l_node = GlobalToLocal::buildNode(mesh);
    auto g2l_cell = GlobalToLocal::buildCell(mesh);
    Parfait::syncVector(mp, new_cell_owners, g2l_cell, cell_sync_pattern);
    auto fragments_to_send = queueFragmentsToSendBasedOnCells(mp, mesh, new_cell_owners);
    auto fragments = packFragments(fragments_to_send);
    fragments = mp.Exchange(fragments);

    TinfMeshAppender final_mesh;
    for (auto& pair : fragments) {
        TinfMeshData frag_data;
        frag_data.unpack(pair.second);
        TinfMesh frag(std::move(frag_data), mp.Rank());
        for (int c = 0; c < frag.cellCount(); c++) {
            final_mesh.addCell(frag.getCell(c));
        }
    }
    auto out_mesh = std::make_shared<TinfMesh>(final_mesh.getData(), mp.Rank());
    inf::setNodeOwners(mp, out_mesh);
    auto tags = inf::extractAll2DTags(mp, mesh);
    for (auto t : tags) {
        out_mesh->setTagName(t, mesh.tagName(t));
    }
    return out_mesh;
}

std::shared_ptr<inf::TinfMesh> inf::MeshShuffle::shuffleNodes(MessagePasser mp,
                                                              const inf::MeshInterface& mesh,
                                                              std::vector<int> new_node_owners) {
    TRACER_PROFILE_FUNCTION
    throwIfCannot_nodes(mp, mesh, new_node_owners);

    Parfait::SyncPattern node_sync_pattern = buildNodeSyncPattern(mp, mesh);
    auto g2l = GlobalToLocal::buildNode(mesh);
    Parfait::syncVector(mp, new_node_owners, g2l, node_sync_pattern);

    auto fragments_to_send = queueFragmentsToSendBasedOnNodes(mp, mesh, g2l, new_node_owners);
    auto fragments = packFragments(fragments_to_send);
    fragments = mp.Exchange(fragments);
    TinfMeshAppender final_mesh;
    for (auto& pair : fragments) {
        TinfMeshData frag_data;
        frag_data.unpack(pair.second);
        TinfMesh frag(std::move(frag_data), mp.Rank());
        for (int c = 0; c < frag.cellCount(); c++) {
            final_mesh.addCell(frag.getCell(c));
        }
    }
    auto out_mesh = std::make_shared<TinfMesh>(final_mesh.getData(), mp.Rank());
    auto tags = inf::extractAll2DTags(mp, mesh);
    for (auto t : tags) {
        out_mesh->setTagName(t, mesh.tagName(t));
    }
    g2l = GlobalToLocal::buildNode(*out_mesh);
    inferNewCellOwners(*out_mesh, g2l);
    return out_mesh;
}
std::vector<int> MeshShuffle::repartitionCells(MessagePasser mp,
                                               const MeshInterface& mesh,
                                               const std::vector<double>& cell_costs) {
    TRACER_PROFILE_FUNCTION
    std::vector<Parfait::Point<double>> xyz;
    std::vector<double> weights;
    std::vector<int> owned_cell_ids;
    for (int cell_id = 0; cell_id < mesh.cellCount(); ++cell_id) {
        if (mesh.ownedCell(cell_id)) {
            inf::Cell cell(mesh, cell_id);
            xyz.push_back(cell.averageCenter());
            weights.push_back(cell_costs[cell_id]);
            owned_cell_ids.push_back(cell_id);
        }
    }
    auto compact_part =
        Parfait::recursiveBisection(mp, xyz, weights, mp.NumberOfProcesses(), 1.0e-4);

    std::vector<int> part(mesh.cellCount());
    for (size_t i = 0; i < compact_part.size(); ++i) {
        part[owned_cell_ids[i]] = compact_part[i];
    }

    auto g2l_cell = GlobalToLocal::buildCell(mesh);
    auto cell_sync_pattern = buildCellSyncPattern(mp, mesh);
    Parfait::syncVector(mp, part, g2l_cell, cell_sync_pattern);

    return part;
}
std::vector<int> MeshShuffle::repartitionNodes(MessagePasser mp,
                                               const MeshInterface& mesh,
                                               const std::vector<double>& node_costs) {
    TRACER_PROFILE_FUNCTION
    std::vector<Parfait::Point<double>> xyz;
    std::vector<double> weights;
    std::vector<int> owned_node_ids;
    for (int node_id = 0; node_id < mesh.nodeCount(); ++node_id) {
        if (mesh.ownedNode(node_id)) {
            xyz.push_back(mesh.node(node_id));
            weights.push_back(node_costs[node_id]);
            owned_node_ids.push_back(node_id);
        }
    }
    auto compact_part =
        Parfait::recursiveBisection(mp, xyz, weights, mp.NumberOfProcesses(), 1.0e-4);

    std::vector<int> part(mesh.nodeCount());
    for (size_t i = 0; i < compact_part.size(); ++i) {
        part[owned_node_ids[i]] = compact_part[i];
    }

    // Sync the part vector to avoid confusing ghost entries
    auto g2l_node = GlobalToLocal::buildNode(mesh);
    auto node_sync_pattern = buildNodeSyncPattern(mp, mesh);
    Parfait::syncVector(mp, part, g2l_node, node_sync_pattern);

    return part;
}
std::vector<int> MeshShuffle::repartitionNodes(MessagePasser mp, const MeshInterface& mesh) {
    TRACER_PROFILE_FUNCTION
    std::vector<double> fake_node_costs(mesh.nodeCount(), 1.0);
    return repartitionNodes(mp, mesh, fake_node_costs);
}
std::shared_ptr<TinfMesh> buildMeshFromFragments(
    const MessagePasser& mp, const std::map<int, TinfMeshAppender>& fragments_to_send) {
    TRACER_PROFILE_FUNCTION
    auto fragments = packFragments(fragments_to_send);
    fragments = mp.Exchange(fragments);
    TinfMeshAppender final_mesh;
    for (auto& pair : fragments) {
        TinfMeshData frag_data;
        frag_data.unpack(pair.second);
        TinfMesh frag(std::move(frag_data), mp.Rank());
        for (int c = 0; c < frag.cellCount(); c++) {
            final_mesh.addCell(frag.getCell(c));
        }
    }
    return std::make_shared<TinfMesh>(final_mesh.getData(), mp.Rank());
}

std::shared_ptr<inf::TinfMesh> MeshShuffle::extendNodeSupport(const MessagePasser& mp,
                                                              const MeshInterface& mesh) {
    TRACER_PROFILE_FUNCTION
    auto g2l = GlobalToLocal::buildNode(mesh);
    auto fragments_to_send = queueFragmentsToAddNodeStencilSupport(mp, mesh, g2l);
    return buildMeshFromFragments(mp, fragments_to_send);
}
std::shared_ptr<inf::TinfMesh> MeshShuffle::extendCellSupport(const MessagePasser& mp,
                                                              const MeshInterface& mesh) {
    TRACER_PROFILE_FUNCTION
    auto g2l = GlobalToLocal::buildNode(mesh);
    auto fragments_to_send = ensureCellSupport(mp, mesh, g2l);
    return buildMeshFromFragments(mp, fragments_to_send);
}

std::vector<int> MeshShuffle::repartitionCells(MessagePasser mp, const MeshInterface& mesh) {
    TRACER_PROFILE_FUNCTION
    std::vector<double> fake_cost(mesh.cellCount(), 1.0);
    return repartitionCells(mp, mesh, fake_cost);
}
std::shared_ptr<inf::TinfMesh> MeshShuffle::claimCellStencils(
    MessagePasser mp, const MeshInterface& mesh, std::set<int> cell_stencils_to_claim) {
    auto c2c = inf::CellToCell::build(mesh);
    std::map<int, std::vector<long>> I_want_these_cells_from_you;
    for (int stencil : cell_stencils_to_claim) {
        int owner = mesh.cellOwner(stencil);
        auto global = mesh.globalCellId(stencil);
        Parfait::VectorTools::insertUnique(I_want_these_cells_from_you[owner], global);
    }

    auto you_want_these_cells_from_me = mp.Exchange(I_want_these_cells_from_you);
    I_want_these_cells_from_you.clear();

    struct Request {
        int rank;
        long gid;
        bool operator<(const Request& r) const {
            if (rank < r.rank) return true;
            if (gid < r.gid) return true;
            return false;
        }
        bool operator==(const Request& r) const { return r.rank == rank and r.gid == gid; }
    };

    std::map<int, std::vector<Request>> bro_needs_these_cells;
    auto g2l = inf::GlobalToLocal::buildCell(mesh);
    for (auto& pair : you_want_these_cells_from_me) {
        int bro = pair.first;
        for (long gid : pair.second) {
            int lid = g2l.at(gid);
            for (auto neighbor : c2c[lid]) {
                long neighbor_gid = mesh.globalCellId(neighbor);
                int neighbor_owner = mesh.cellOwner(neighbor);
                Parfait::VectorTools::insertUnique(bro_needs_these_cells[neighbor_owner],
                                                   Request{bro, neighbor_gid});
            }
            Parfait::VectorTools::insertUnique(bro_needs_these_cells[mp.Rank()], {bro, gid});
        }
    }
    auto bro_needs_these_from_me = mp.Exchange(bro_needs_these_cells);

    int UNASSIGNED = -1;
    std::vector<int> cell_part_vector(mesh.cellCount(), UNASSIGNED);

    auto reassign_cell = [&](int cell_id, int new_owner) {
        cell_part_vector[cell_id] = std::max(cell_part_vector[cell_id], new_owner);
    };

    for (auto& pair : bro_needs_these_from_me) {
        for (auto& request : pair.second) {
            int rank = request.rank;
            long global = request.gid;
            PARFAIT_ASSERT_KEY(g2l, global);
            auto local = g2l.at(global);
            reassign_cell(local, rank);
        }
    }

    for (int c = 0; c < mesh.cellCount(); c++) {
        if (cell_part_vector[c] == UNASSIGNED) cell_part_vector[c] = mesh.cellOwner(c);
    }
    return shuffleCells(mp, mesh, cell_part_vector);
}
std::shared_ptr<inf::TinfMesh> MeshShuffle::repartitionByVolumeCells(
    MessagePasser mp, const MeshInterface& mesh, const std::vector<double>& cell_cost) {
    auto volume_cell_dimensionality = maxCellDimensionality(mp, mesh);
    auto surface_cell_dimensionality = volume_cell_dimensionality - 1;
    std::vector<Parfait::Point<double>> xyz;
    std::vector<double> weights;
    std::vector<int> assigned_cell_ids;
    for (int cell_id = 0; cell_id < mesh.cellCount(); ++cell_id) {
        if (mesh.cellDimensionality(cell_id) == volume_cell_dimensionality) {
            if (mesh.ownedCell(cell_id)) {
                inf::Cell cell(mesh, cell_id);
                xyz.push_back(cell.averageCenter());
                weights.push_back(cell_cost[cell_id]);
                assigned_cell_ids.push_back(cell_id);
            }
        }
    }
    auto compact_part =
        Parfait::recursiveBisection(mp, xyz, weights, mp.NumberOfProcesses(), 1.0e-4);

    std::vector<int> part(mesh.cellCount());
    for (size_t i = 0; i < compact_part.size(); ++i) {
        part[assigned_cell_ids[i]] = compact_part[i];
    }

    auto face_neighbors =
        FaceNeighbors::buildOnlyForCellsWithDimensionality(mesh, surface_cell_dimensionality);
    FieldTools::setSurfaceCellToVolumeNeighborValue(
        mesh, face_neighbors, surface_cell_dimensionality, part);

    return shuffleCells(mp, mesh, part);
}
std::shared_ptr<inf::TinfMesh> MeshShuffle::repartitionByVolumeCells(MessagePasser mp,
                                                                     const MeshInterface& mesh) {
    return repartitionByVolumeCells(mp, mesh, std::vector<double>(mesh.cellCount(), 1.0));
}
std::vector<int> MeshShuffle::repartitionNodes4D(MessagePasser mp,
                                                 const MeshInterface& mesh,
                                                 const std::vector<double>& fourth_dimension) {
    std::vector<Parfait::Point<double, 4>> points(mesh.nodeCount());
    for (int n = 0; n < mesh.nodeCount(); n++) {
        Parfait::Point<double> p = mesh.node(n);
        points[n][0] = p[0];
        points[n][1] = p[1];
        points[n][2] = p[2];
        points[n][3] = fourth_dimension[n];
    }
    return Parfait::recursiveBisection(mp, points, mp.NumberOfProcesses(), 1.0e-4);
}
