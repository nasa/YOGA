#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include <set>
#include <t-infinity/CellSelectedMesh.h>


TEST_CASE("remap selected global ids more scalable"){
    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    std::vector<long> selection;
    std::vector<int> owner;
    if (mp.Rank() == 0) {
        selection = {0, 1, 5};
        owner = {0,0,0};
    } else {
        selection = {5, 7, 8};
        owner = {0,1,1};
    }
    auto old_to_new_gids = inf::mapNewGlobalIdsFromSparseSelection(mp, selection, owner);
    REQUIRE(old_to_new_gids.size() == selection.size());

    if (mp.Rank() == 0) {
        REQUIRE(old_to_new_gids.at(0) == 0);
        REQUIRE(old_to_new_gids.at(1) == 1);
        REQUIRE(old_to_new_gids.at(2) == 2);
    } else {
        REQUIRE(old_to_new_gids.at(0) == 2);
        REQUIRE(old_to_new_gids.at(1) == 3);
        REQUIRE(old_to_new_gids.at(2) == 4);
    }
}