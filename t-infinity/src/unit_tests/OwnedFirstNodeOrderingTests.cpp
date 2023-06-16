#include <RingAssertions.h>
#include <t-infinity/ReorderMesh.h>

TEST_CASE("Reorder so owned nodes are first") {
    std::vector<bool> owned = {true, true, false, false, false, true, true, true};
    size_t num_nodes = owned.size();
    std::vector<int> old_to_new = inf::MeshReorder::buildReorderNodeOwnedFirst(owned);
    REQUIRE(old_to_new.size() == num_nodes);
    std::vector<int> expected_ordering = {0, 1, 5, 6, 7, 2, 3, 4};
    REQUIRE(old_to_new == expected_ordering);
}
