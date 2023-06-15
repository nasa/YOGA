#include <RingAssertions.h>
#include <parfait/UniquePoints.h>

TEST_CASE("Unique points tests") {
    Parfait::UniquePoints unique({{0, 0, 0}, {1, 1, 1}});

    REQUIRE(unique.getNodeId({.1, .1, .1}) == 0);
    REQUIRE(unique.getNodeId({.1, .1, .1}) == 0);
    REQUIRE(unique.getNodeId({.1, .2, .1}) == 1);
    REQUIRE(unique.points.size() == 2);
}
