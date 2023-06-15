#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>

std::map<long, std::set<long>> augmentNeighborsOfNeighborsLocally(const std::map<long, std::set<long>>& node_to_node) {
    std::map<long, std::set<long>> augmented_n2n = node_to_node;

    for (auto center_to_neighbors : node_to_node) {
        auto neighbors = center_to_neighbors.second;
        for (auto n : neighbors) {
            for (auto n1 : neighbors) {
                if (n1 == n) continue;
                augmented_n2n[n].insert(n1);
            }
        }
    }

    return augmented_n2n;
}

template <typename GetOwner>
std::map<long, std::set<long>> calcNeighborsOfNeighbors(MessagePasser mp,
                                                        const std::map<long, std::set<long>>& node_to_node,
                                                        GetOwner get_owner) {
    typedef int Rank;
    typedef std::map<long, std::set<long>> Stencils;
    std::map<Rank, std::map<long, std::set<long>>> stencils_to_send_to_ranks;
    auto augmented_node_to_node = augmentNeighborsOfNeighborsLocally(node_to_node);

    for (auto& node_and_stencil : node_to_node) {
        auto center_node = node_and_stencil.first;
        auto& stencil = node_and_stencil.second;
        auto center_owner = get_owner(center_node);
        if (mp.Rank() == center_owner) {
            for (auto neighbor : stencil) {
                auto neighbor_owner = get_owner(neighbor);
                for (auto n : stencil) {
                    if (n == neighbor) continue;
                    stencils_to_send_to_ranks[neighbor_owner][neighbor].insert(n);
                }
            }
        }
    }

    auto packer = [&](MessagePasser::Message& msg, const Stencils& s) { msg.pack(s); };
    auto unpacker = [&](MessagePasser::Message& msg, Stencils& s) { msg.unpack(s); };
    auto incoming_stencils = mp.Exchange(stencils_to_send_to_ranks, packer, unpacker);
    for (auto& rank_to_incoming_stencils : incoming_stencils) {
        auto& stencils = rank_to_incoming_stencils.second;
        for (auto& node_to_stencil : stencils) {
            auto node = node_to_stencil.first;
            auto& node_stencil = node_to_stencil.second;
            for (auto neighbor : node_stencil) {
                augmented_node_to_node[node].insert(neighbor);
            }
        }
    }

    return augmented_node_to_node;
}

TEST_CASE("can add layer of halo") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    std::map<long, std::set<long>> sparsity_in_global_ids;
    std::map<long, int> owned_by;
    if (mp.Rank() == 0) {
        owned_by[100] = 0;
        owned_by[101] = 0;
        owned_by[102] = 1;
        sparsity_in_global_ids[100] = {101};
        sparsity_in_global_ids[101] = {100, 102};
    } else if (mp.Rank() == 1) {
        owned_by[101] = 0;
        owned_by[102] = 1;
        owned_by[103] = 1;
        sparsity_in_global_ids[102] = {101, 103};
        sparsity_in_global_ids[103] = {102};
    }

    auto getOwner = [&](long global_id) { return owned_by.at(global_id); };

    auto neighbors_of_neighbors = calcNeighborsOfNeighbors(mp, sparsity_in_global_ids, getOwner);
    if (mp.Rank() == 0) {
        REQUIRE(neighbors_of_neighbors.at(100) == std::set<long>{101, 102});
        REQUIRE(neighbors_of_neighbors.at(101) == std::set<long>{100, 102, 103});
    } else if (mp.Rank() == 1) {
        REQUIRE(neighbors_of_neighbors.at(102) == std::set<long>{100, 101, 103});
        REQUIRE(neighbors_of_neighbors.at(103) == std::set<long>{101, 102});
    }
}