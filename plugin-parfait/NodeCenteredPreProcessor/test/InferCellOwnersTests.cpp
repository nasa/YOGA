#include <RingAssertions.h>
#include <Redistributor.h>

TEST_CASE("Extract node owners in cell") {
    std::vector<int> node_owners = {100, 100, 100, 100, 0, 1, 1, 1, 1, 2};
    std::vector<int> cell = {9, 8, 7, 6, 5, 4};
    auto owners = Redistributor::getCellNodeOwners(cell, node_owners);
    REQUIRE(owners.size() == cell.size());
    REQUIRE(owners[0] == 2);
    REQUIRE(owners[1] == 1);
    REQUIRE(owners[2] == 1);
    REQUIRE(owners[3] == 1);
    REQUIRE(owners[4] == 1);
    REQUIRE(owners[5] == 0);
}
