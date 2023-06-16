#include <RingAssertions.h>
#include <t-infinity/MeshMover.h>

using namespace inf;

TEST_CASE("Reflect a point"){
    Parfait::Point<double> p {1,2,3};
    double offset = 0.0;

    SECTION("x-reflection (about yz plane)") {
        Parfait::Point<double> plane_normal{1, 0, 0};
        auto reflected = MeshMover::reflect(p, plane_normal, offset);

        Parfait::Point<double> expected{-1, 2, 3};
        REQUIRE(expected.approxEqual(reflected));
    }
    SECTION("y-reflection (about xz plane)") {
        Parfait::Point<double> plane_normal{0, 1, 0};
        auto reflected = MeshMover::reflect(p, plane_normal, offset);

        Parfait::Point<double> expected{1, -2, 3};
        REQUIRE(expected.approxEqual(reflected));
    }
    SECTION("z-reflection (about xy plane)") {
        Parfait::Point<double> plane_normal{0, 0, 1};
        auto reflected = MeshMover::reflect(p, plane_normal, offset);

        Parfait::Point<double> expected{1, 2, -3};
        REQUIRE(expected.approxEqual(reflected));
    }

}
