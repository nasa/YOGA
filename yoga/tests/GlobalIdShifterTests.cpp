#include <RingAssertions.h>
#include "GlobalIdShifter.h"

using namespace YOGA;

TEST_CASE("Calc offsets to assign new ordinals to global ids on each component"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<long> owned_gids {0};
    int component_grid_id = mp.Rank();

    SECTION("count nodes per grid") {
        auto nodes_per_component = GlobalIdShifter::countNodesPerComponentGrid(mp, owned_gids, component_grid_id);
        REQUIRE(mp.NumberOfProcesses() == int(nodes_per_component.size()));
        for (long n : nodes_per_component) REQUIRE(1 == n);
    }

    SECTION("calc offset for each component"){
        long offset = GlobalIdShifter::calcGlobalIdOffsetForComponentGrid(mp,owned_gids,component_grid_id);
        const long expected = mp.Rank();
        REQUIRE(expected == offset);
    }
}

TEST_CASE("Convert node counts to offsets"){
    std::vector<long> nodes_per_grid {3,7,4};
    auto offsets = GlobalIdShifter::convertNodeCountsToOffsets(nodes_per_grid);
    REQUIRE(nodes_per_grid.size() == offsets.size());

    std::vector<long> expected {0,3,10};
    REQUIRE(expected == offsets);
}