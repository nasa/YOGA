#include <RingAssertions.h>
#include <IsotropicSpacingTree.h>

using namespace Parfait;
using namespace YOGA;

TEST_CASE("insert one item"){
    Extent<double> extent({0,0,0}, {1,1,1});
    IsotropicSpacingTree tree(extent);

    Point<double> p {0.1,0.1,0.1};
    Extent<double> e {p,p};
    double spacing = 0.25;
    tree.insert(e,spacing);

    SECTION("at the point submitted, the spacing should be at least as small as requested") {
        double s = tree.getSpacingAt(p);
        double expected_ceil = spacing;
        REQUIRE(expected_ceil >= s);
        double expected_floor = .5*spacing;
        REQUIRE(expected_floor <= s);
    }
    SECTION("at the center, the spacing should be twice the minimum"){
        double s = tree.getSpacingAt({.5,.5,.5});
        double expected = 2*spacing;
        REQUIRE(expected >= s);
        double expected_floor = spacing;
        REQUIRE(expected_floor <= s);
    }
    SECTION("at the top corner, no refinement should be required based on geometric growth"){
        double s = tree.getSpacingAt({1.,1.,1.});
        double expected = 1.0;
        REQUIRE(expected == s);
    }
}

TEST_CASE("when no items are inserted, spacing should equal the domain size"){
    Extent<double> extent({0,0,0}, {1,1,1});
    IsotropicSpacingTree tree(extent);
    double s = tree.getSpacingAt({0,0,0});
    REQUIRE(1.0 == s);
}


TEST_CASE("Calculate octants from extent"){
    Extent<double> e {{0,0,0},{1,1,1}};
    Voxel voxel(e);

    SECTION("bottom octants") {
        auto octant = voxel.octant(0);
        auto expected = e.lo;
        REQUIRE(expected == octant.lo);
        expected = {.5, .5, .5};
        REQUIRE(expected == octant.hi);

        octant = voxel.octant(1);
        expected = {.5, 0, 0};
        REQUIRE(expected == octant.lo);
        expected = {1, .5, .5};
        REQUIRE(expected == octant.hi);

        octant = voxel.octant(2);
        expected = {.5, .5, 0};
        REQUIRE(expected == octant.lo);
        expected = {1, 1, .5};
        REQUIRE(expected == octant.hi);

        octant = voxel.octant(3);
        expected = {0, .5, 0};
        REQUIRE(expected == octant.lo);
        expected = {.5, 1, .5};
        REQUIRE(expected == octant.hi);
    }
    SECTION("top octants") {
        auto octant = voxel.octant(4);
        Point<double> expected {0,0,.5};
        REQUIRE(expected == octant.lo);
        expected = {.5, .5, 1};
        REQUIRE(expected == octant.hi);

        octant = voxel.octant(5);
        expected = {.5, 0, .5};
        REQUIRE(expected == octant.lo);
        expected = {1, .5, 1};
        REQUIRE(expected == octant.hi);

        octant = voxel.octant(6);
        expected = {.5, .5, .5};
        REQUIRE(expected == octant.lo);
        expected = {1, 1, 1};
        REQUIRE(expected == octant.hi);

        octant = voxel.octant(7);
        expected = {0, .5, .5};
        REQUIRE(expected == octant.lo);
        expected = {.5, 1, 1};
        REQUIRE(expected == octant.hi);
    }
}