#include <RingAssertions.h>
#include <t-infinity/MapBcFileCombiner.h>

using namespace inf;

TEST_CASE("Combine mapbc files") {
    Parfait::BoundaryConditionMap map_a, map_b;
    map_a[9] = {5000, "Farfield"};
    map_a[2] = {3000, "Some wall"};

    map_b[1] = {9001, "Wall of Goku"};
    map_b[3] = {-1, "A negative boundary"};

    auto combined = MapBcCombiner::combine({map_a, map_b});

    REQUIRE(4 == combined.size());

    // Make sure tags are ordinals starting at 1 (required for Fun3D)
    REQUIRE(1 == combined.count(1));
    REQUIRE(1 == combined.count(2));
    REQUIRE(1 == combined.count(3));
    REQUIRE(1 == combined.count(4));

    REQUIRE(3000 == combined.at(1).first);
    REQUIRE(5000 == combined.at(2).first);
    REQUIRE(9001 == combined.at(3).first);
    REQUIRE(-1 == combined.at(4).first);
}
