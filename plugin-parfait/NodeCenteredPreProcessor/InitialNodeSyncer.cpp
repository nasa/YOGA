#include "InitialNodeSyncer.h"

InitialNodeSyncer::InitialNodeSyncer(MessagePasser mp) : mp(mp) {}

std::vector<std::vector<int>> InitialNodeSyncer::createSendNodeMap(int num_ranks, const std::vector<int>& part) const {
    std::vector<std::vector<int>> send_local_nodes(num_ranks);
    for (int local_node = 0; local_node < long(part.size()); local_node++) {
        int owner = part[local_node];
        send_local_nodes.at(owner).push_back(local_node);
    }
    return send_local_nodes;
}

std::vector<long> InitialNodeSyncer::syncOwnedGlobalNodes(const std::vector<long>& global_node_ids,
                                                          const std::vector<int>& part) {
    auto send_nodes = createSendNodeMap(mp.NumberOfProcesses(), part);
    std::vector<std::vector<long>> send_global_nodes(send_nodes.size());
    for (size_t r = 0; r < send_nodes.size(); r++) {
        for (size_t i = 0; i < send_nodes[r].size(); i++)
            send_global_nodes[r].push_back(global_node_ids[send_nodes[r][i]]);
    }
    auto gids_from_ranks = mp.Exchange(send_global_nodes);
    std::vector<long> my_gids;
    for (auto& gids : gids_from_ranks) {
        for (long gid : gids) my_gids.push_back(gid);
    }
    return my_gids;
}
