#include <MessagePasser/MessagePasser.h>
#include <parfait/Throw.h>
#include <t-infinity/FaceNeighbors.h>
#include <t-infinity/GlobalPairIds.h>
#include <RingAssertions.h>
#include "MockMeshes.h"

TEST_CASE("Queue pairs to send to owners") {
    std::vector<std::array<long, 2>> pairs = {{1, 100}, {2, 100}, {1, 2}};
    std::map<long, int> owners = {{1, 0}, {2, 4}, {100, 4}};
    auto get_owner = [&](long id) { return owners.at(id); };
    auto queued = inf::impl_for_testing::queuePairsToSendToOwners(pairs, get_owner);
    REQUIRE(queued.size() == 2);
    REQUIRE(queued.at(0).size() == 2);
    REQUIRE_THROWS(queued.at(1).size() == 0);
    REQUIRE(queued.at(4).size() == 1);
}

TEST_CASE("Can use an array as a key into a map") {
    std::array<long, 2> a = {0, 1};
    std::map<std::array<long, 2>, int> map;
    map[a] = 1;
    map[std::array<long, 2>{3, 9}] = 2;

    REQUIRE(map.size() == 2);
    REQUIRE(map[std::array<long, 2>{3, 9}] == 2);
}

TEST_CASE("Flatten and make unique") {
    std::map<int, std::vector<std::array<long, 2>>> pairs_from_ranks_mock;
    pairs_from_ranks_mock[0] = {{8, 9}, {10, 11}};
    pairs_from_ranks_mock[1] = {{8, 9}, {12, 13}};

    auto unique_pairs = inf::impl_for_testing::flattenAndMakeUnique(pairs_from_ranks_mock);
    REQUIRE(unique_pairs.size() == 3);
    REQUIRE(unique_pairs.count({8, 9}) == 1);
    REQUIRE(unique_pairs.count({10, 11}) == 1);
    REQUIRE(unique_pairs.count({12, 13}) == 1);
}

TEST_CASE("Calc my start offset") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto offset = inf::impl_for_testing::calcMyStartOffset(mp, 1);
    REQUIRE(offset == mp.Rank());
}

TEST_CASE("Assign ids for owned pairs") {
    std::set<std::array<long, 2>> pairs;
    pairs.insert({8, 9});
    pairs.insert({10, 11});

    long starting_offset = 8;
    auto pair_to_id = inf::impl_for_testing::assignIdsForOwnedPairs(pairs, starting_offset);
    REQUIRE(pair_to_id.size() == 2);
    REQUIRE(pair_to_id.at({8, 9}) == 8);
    REQUIRE(pair_to_id.at({10, 11}) == 9);
}

TEST_CASE("Consistent Global Pairs") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;

    printf("Running Consistent global pairs");
    std::vector<std::array<long, 2>> pairs_I_have_in_global_indexing;
    if (mp.Rank() == 0) {
        pairs_I_have_in_global_indexing.push_back({2, 4});
        pairs_I_have_in_global_indexing.push_back({2, 7});
    } else if (mp.Rank() == 1) {
        pairs_I_have_in_global_indexing.push_back({1, 2});
        pairs_I_have_in_global_indexing.push_back({1, 4});
    }

    long max_pair_entry = 8;  // all pair entries are smaller than this number

    auto pair_to_global_pair_id =
        inf::buildGlobalPairIds(mp.getCommunicator(), pairs_I_have_in_global_indexing, max_pair_entry);
    REQUIRE(pair_to_global_pair_id.size() == 2);
    if (mp.Rank() == 0) {
        REQUIRE(pair_to_global_pair_id.at({2, 4}) == 2);
        REQUIRE(pair_to_global_pair_id.at({2, 7}) == 3);
    } else if (mp.Rank() == 1) {
        REQUIRE(pair_to_global_pair_id.at({1, 2}) == 0);
        REQUIRE(pair_to_global_pair_id.at({1, 4}) == 1);
    }
}
