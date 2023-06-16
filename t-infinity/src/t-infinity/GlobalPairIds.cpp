#include "GlobalPairIds.h"
#include <parfait/Throw.h>
#include <parfait/LinearPartitioner.h>

namespace inf {

namespace impl_for_testing {
    std::map<int, std::vector<std::array<long, 2>>> queuePairsToSendToOwners(
        const std::vector<std::array<long, 2>>& pairs, std::function<int(long)> getOwner) {
        typedef std::array<long, 2> Pair;
        typedef int Rank;
        std::map<Rank, std::vector<Pair>> send_pair_to_rank;
        for (auto& pair : pairs) {
            int owner = getOwner(pair[0]);
            send_pair_to_rank[owner].push_back(pair);
        }
        return send_pair_to_rank;
    }

    std::set<std::array<long, 2>> flattenAndMakeUnique(
        const std::map<int, std::vector<std::array<long, 2>>>& incoming_pairs) {
        std::set<std::array<long, 2>> unique_pairs;
        for (auto& loop : incoming_pairs) {
            auto& incoming_from_rank = loop.second;
            for (auto& pair : incoming_from_rank) {
                unique_pairs.insert(pair);
            }
        }
        return unique_pairs;
    }

    long calcMyStartOffset(const MessagePasser& mp, long num_owned) {
        long start_id = 0;
        auto pair_count_per_rank = mp.Gather(num_owned);
        for (int r = 0; r < mp.Rank(); r++) {
            start_id += pair_count_per_rank[r];
        }
        return start_id;
    }

    std::map<std::array<long, 2>, long> assignIdsForOwnedPairs(
        const std::set<std::array<long, 2>>& pairs, long next_global_id) {
        std::map<std::array<long, 2>, long> pair_to_global_id;
        for (auto& pair : pairs) {
            pair_to_global_id[pair] = next_global_id++;
        }
        return pair_to_global_id;
    }

    void throwIfPairsAreNotSorted(const std::vector<std::array<long, 2>>& pairs) {
        for (auto& p : pairs) {
            if (p[0] >= p[1])
                PARFAIT_THROW("Pairs must be sorted to build consistent and unique global ids.\n");
        }
    }
}

std::map<std::array<long, 2>, long> buildGlobalPairIds(
    MPI_Comm comm, const std::vector<std::array<long, 2>>& pairs, long max_number_nodes) {
    using namespace impl_for_testing;
    MessagePasser mp(comm);
    typedef int Rank;
    typedef long GlobalId;
    typedef std::array<long, 2> Pair;
    impl_for_testing::throwIfPairsAreNotSorted(pairs);

    auto get_owner = [&](long pair_entry) {
        return Parfait::LinearPartitioner::getWorkerOfWorkItem(
            pair_entry, max_number_nodes, mp.NumberOfProcesses());
    };

    std::map<Rank, std::vector<Pair>> pairs_to_send_to_ranks =
        queuePairsToSendToOwners(pairs, get_owner);

    auto incoming_pairs = mp.Exchange(pairs_to_send_to_ranks);
    std::set<Pair> unique_pairs_i_own = flattenAndMakeUnique(incoming_pairs);
    auto my_pair_start_id = calcMyStartOffset(mp, unique_pairs_i_own.size());
    std::map<Pair, long> assigned_ids_for_owned_pairs =
        assignIdsForOwnedPairs(unique_pairs_i_own, my_pair_start_id);

    std::map<Rank, std::vector<GlobalId>> return_pair_global_ids_for_rank;
    for (auto& p : incoming_pairs) {
        auto rank = p.first;
        for (auto& pair : p.second) {
            long global = assigned_ids_for_owned_pairs.at(pair);
            return_pair_global_ids_for_rank[rank].push_back(global);
        }
    }

    auto my_pairs_global_ids = mp.Exchange(return_pair_global_ids_for_rank);

    std::map<Pair, GlobalId> global_pair_ids;
    for (auto& p : my_pairs_global_ids) {
        auto r = p.first;
        if (pairs_to_send_to_ranks.at(r).size() != my_pairs_global_ids.at(r).size())
            PARFAIT_THROW("Received different number of pair global Ids then I asked for.");
        for (int i = 0; i < int(pairs_to_send_to_ranks.at(r).size()); i++) {
            long global = my_pairs_global_ids.at(r)[i];
            auto pair = pairs_to_send_to_ranks.at(r)[i];
            global_pair_ids[pair] = global;
        }
    }
    return global_pair_ids;
}
}