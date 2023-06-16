#include <RingAssertions.h>
#include <parfait/Region.h>

using namespace Parfait;

TEST_CASE("intersectable sphere") {
    SphereRegion s({0, 0, 0}, 1.0);
    REQUIRE(s.contains({0.1, 0.1, 0.1}));
    REQUIRE_FALSE(s.contains({2, 2, 2}));
}

TEST_CASE("intersectable tet") {
    auto corners = std::array<Parfait::Point<double>, 4>{{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    TetRegion tet(corners);
    REQUIRE(tet.contains({.1, .1, .1}));
    REQUIRE_FALSE(tet.contains({.1, .1, -.1}));
    REQUIRE_FALSE(tet.contains({-.1, .1, .1}));
    REQUIRE_FALSE(tet.contains({.1, -.1, .1}));
    REQUIRE_FALSE(tet.contains({.7, .7, .7}));
}

TEST_CASE("intersectable hex") {
    std::array<Parfait::Point<double>, 8> corners;
    corners[0] = {0, 0, 0};
    corners[1] = {1, 0, 0};
    corners[2] = {1, 1, 0};
    corners[3] = {0, 1, 0};
    corners[4] = {1, 0, 2};
    corners[5] = {2, 0, 2};
    corners[6] = {2, 1, 2};
    corners[7] = {1, 1, 2};

    HexRegion hex(corners);
    REQUIRE_FALSE(hex.contains({5, 5, 5}));
    REQUIRE(hex.contains({.4, .4, .2}));
    REQUIRE_FALSE(hex.contains({.1, .1, 1.9}));
}