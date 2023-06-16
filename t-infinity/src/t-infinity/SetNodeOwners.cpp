#include "SetNodeOwners.h"
#include <parfait/Throw.h>

namespace inf {

long min(const std::vector<long>& vec) {
    long min_entry = std::numeric_limits<long>::max();
    for (auto e : vec) {
        min_entry = std::min(e, min_entry);
    }
    return min_entry;
}

inline std::set<int> extractNodesInOwnedCells(const std::shared_ptr<inf::TinfMesh>& mesh) {
    int num_cells = mesh->cellCount();
    std::set<int> nodes_in_owned_cells;
    std::vector<int> cell_nodes;
    for (int c = 0; c < num_cells; c++) {
        if (mesh->ownedCell(c)) {
            cell_nodes.resize(mesh->cellTypeLength(mesh->cellType(c)));
            mesh->cell(c, cell_nodes.data());
            for (int n : cell_nodes) nodes_in_owned_cells.insert(n);
        }
    }
    return nodes_in_owned_cells;
}
inline std::vector<std::vector<long>> convertToGlobalCellId(
    inf::TinfMesh& mesh, const std::vector<std::vector<int>>& n2c) {
    std::vector<std::vector<long>> n2c_global(n2c.size());
    for (size_t i = 0; i < n2c.size(); i++) {
        for (int local : n2c[i]) {
            long global = mesh.globalCellId(local);
            n2c_global[i].push_back(global);
        }
    }

    return n2c_global;
}
void setNodeOwners(MessagePasser mp, std::shared_ptr<inf::TinfMesh> mesh) {
    int UNKNOWN = -1;
    auto n2c_local = inf::NodeToCell::build(*mesh);
    auto n2c = convertToGlobalCellId(*mesh, n2c_local);
    auto g2l_node = inf::GlobalToLocal::buildNode(*mesh);
    auto g2l_cell = inf::GlobalToLocal::buildCell(*mesh);

    auto nodes_we_have_full_stencils_for = extractNodesInOwnedCells(mesh);
    std::vector<int> node_owner(mesh->nodeCount(), UNKNOWN);
    for (int n : nodes_we_have_full_stencils_for) {
        long deciding_cell = min(n2c[n]);
        int local = g2l_cell.at(deciding_cell);
        int owner = mesh->cellOwner(local);
        node_owner[n] = owner;
    }

    std::map<int, std::vector<long>> query_for_ranks;
    for (int n = 0; n < mesh->nodeCount(); n++) {
        if (node_owner[n] == UNKNOWN) {
            int this_cell_knows = n2c_local[n].front();
            int owner_of_knowing_cell = mesh->cellOwner(this_cell_knows);
            query_for_ranks[owner_of_knowing_cell].push_back(mesh->globalNodeId(n));
        }
    }

    auto requests = mp.Exchange(query_for_ranks);
    struct ReplyType {
        long global_id;
        int owner;
    };
    std::map<int, std::vector<ReplyType>> reply_for_ranks;

    for (auto& pair : requests) {
        int rank = pair.first;
        auto& query_from_rank = pair.second;
        for (auto global_node_id : query_from_rank) {
            PARFAIT_ASSERT_KEY(g2l_node, global_node_id);
            int local = g2l_node.at(global_node_id);
            int owner = node_owner[local];
            reply_for_ranks[rank].push_back({global_node_id, owner});
        }
    }

    auto replies = mp.Exchange(reply_for_ranks);

    for (auto& pair : replies) {
        auto& replies_from_rank = pair.second;
        for (auto& reply_pair : replies_from_rank) {
            auto global_node_id = reply_pair.global_id;
            int owner = reply_pair.owner;
            PARFAIT_ASSERT_KEY(g2l_node, global_node_id);
            int local = g2l_node.at(global_node_id);
            node_owner[local] = owner;
        }
    }

    for (auto owner : node_owner) {
        if (owner == UNKNOWN) PARFAIT_THROW("A node owner is still unknown after assignment");
    }

    mesh->mesh.node_owner = node_owner;
}

}