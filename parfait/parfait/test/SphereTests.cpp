#include <RingAssertions.h>
#include <parfait/Sphere.h>

TEST_CASE("Sphere intersects point") {
    Parfait::Sphere s({0, 0, 0}, 1.0);
    REQUIRE(s.intersects({0, 0, 0}));
    REQUIRE_FALSE(s.intersects({2, 0, 0}));
}

TEST_CASE("Sphere intersects extent") {
    Parfait::Sphere s({2, .5, 0}, 1.1);
    Parfait::Extent<double> e{{0, 0, 0}, {1, 1, 1}};

    REQUIRE(s.intersects(e));
    REQUIRE_FALSE(s.intersects(e.lo));
    REQUIRE_FALSE(s.intersects(e.hi));
}
