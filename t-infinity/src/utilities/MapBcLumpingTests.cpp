#include <RingAssertions.h>
#include <parfait/MapbcParser.h>
#include <t-infinity/MapbcLumper.h>

TEST_CASE("lump boundary tags with matching strings"){
    Parfait::BoundaryConditionMap bc_map;
    bc_map[1] = {3000,"wing-surface"};
    bc_map[2] = {3000, "wing-surface"};
    bc_map[3] = {5000, "farfield"};

    auto old_tag_to_new = inf::groupTagsByBoundaryConditionName(bc_map);

    REQUIRE(3 == old_tag_to_new.size());
    REQUIRE(1 == old_tag_to_new[1]);
    REQUIRE(1 == old_tag_to_new[2]);
    REQUIRE(2 == old_tag_to_new[3]);

    SECTION("make a new mapbc with the lumps"){
        auto lumped = inf::lumpBC(bc_map, old_tag_to_new);
        REQUIRE(2 == lumped.size());
        REQUIRE(3000 == lumped[1].first);
        REQUIRE("wing-surface" == lumped[1].second);
        REQUIRE(5000 == lumped[2].first);
        REQUIRE("farfield" == lumped[2].second);
    }
}
