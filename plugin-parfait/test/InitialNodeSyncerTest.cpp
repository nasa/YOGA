#include <MessagePasser/MessagePasser.h>
#include "../NodeCenteredPreProcessor/InitialNodeSyncer.h"
#include <RingAssertions.h>

TEST_CASE("InitialNodeSyncer can create the send node map") {
    MessagePasser mp(MPI_COMM_WORLD);
    InitialNodeSyncer initialNodeSyncer(mp);
    std::vector<int> part = {0, 0, 1, 1};
    auto send_nodes_to_rank = initialNodeSyncer.createSendNodeMap(3, part);
    REQUIRE(send_nodes_to_rank.size() == 3);
    REQUIRE(send_nodes_to_rank[0].size() == 2);
    REQUIRE(send_nodes_to_rank[1].size() == 2);
}

TEST_CASE("sync owned global ids") {
    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    int r = mp.Rank();
    int o = (r == 0) ? (1) : (0);
    std::vector<int> part = {r, r};
    std::vector<long> global_node_ids;
    if (mp.Rank() == 0) global_node_ids = std::vector<long>{0, 1};
    if (mp.Rank() == 1) global_node_ids = std::vector<long>{2, 3};
    InitialNodeSyncer initialNodeSyncer(mp);
    auto send_nodes = initialNodeSyncer.createSendNodeMap(mp.NumberOfProcesses(), part);
    REQUIRE(send_nodes.size() == 2);
    REQUIRE(send_nodes[r].size() == 2);
    REQUIRE(send_nodes[o].size() == 0);

    std::vector<long> new_global_node_ids = initialNodeSyncer.syncOwnedGlobalNodes(global_node_ids, part);
    REQUIRE(new_global_node_ids.size() == 2);
}