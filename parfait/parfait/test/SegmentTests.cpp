#include <RingAssertions.h>
#include <parfait/Segment.h>

TEST_CASE("Segments exist") {
    Parfait::Segment<double> s({0, 0, 0}, {1, 0, 0});
    REQUIRE(s.length() == Approx(1.0));
    REQUIRE(s.getExtent().lo[0] == Approx(0.0));
    REQUIRE(s.getExtent().hi[0] == Approx(1.0));
    REQUIRE(s.getClosestPoint({0, 0, 0}).approxEqual({0, 0, 0}));
    REQUIRE(s.getClosestPoint({1, 0, 0}).approxEqual({1, 0, 0}));
}

TEST_CASE("Segment project") {
    Parfait::Segment<double> s({0, 0, 0}, {1, 0, 0});
    SECTION("end points") {
        REQUIRE(s.getClosestPoint({0, 0, 0}).approxEqual({0, 0, 0}));
        REQUIRE(s.getClosestPoint({1, 0, 0}).approxEqual({1, 0, 0}));
    }
    SECTION("exactly on the line") {
        REQUIRE(s.getClosestPoint({0.5, 0, 0}).approxEqual({0.5, 0, 0}));
        REQUIRE(s.getClosestPoint({0.2, 0, 0}).approxEqual({0.2, 0, 0}));
    }
    SECTION("Near the middle of the line") {
        REQUIRE(s.getClosestPoint({0.5, -0.1, 0}).approxEqual({0.5, 0, 0}));
        REQUIRE(s.getClosestPoint({0.5, .1, .1}).approxEqual({0.5, 0, 0}));
    }
}
