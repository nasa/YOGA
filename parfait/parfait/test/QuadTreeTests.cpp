#include <RingAssertions.h>
#include <parfait/QuadTree.h>
#include <parfait/PointGenerator.h>

TEST_CASE("Quad tree split") {
    auto e = Parfait::Extent<double>({0, 0, 0}, {1, 1, 0.01});
    SECTION("Child 0") {
        Parfait::Extent<double> child0 = Parfait::Quadtree::getChild0Extent(e);
        REQUIRE(child0.lo[0] == Approx(0.0));
        REQUIRE(child0.lo[1] == Approx(0.0));
        REQUIRE(child0.hi[0] == Approx(0.5));
        REQUIRE(child0.hi[1] == Approx(0.5));
    }
    SECTION("Child 1") {
        Parfait::Extent<double> c = Parfait::Quadtree::getChild1Extent(e);
        REQUIRE(c.lo[0] == Approx(0.5));
        REQUIRE(c.lo[1] == Approx(0.0));
        REQUIRE(c.hi[0] == Approx(1.0));
        REQUIRE(c.hi[1] == Approx(0.5));
    }
    SECTION("Child 2") {
        Parfait::Extent<double> c = Parfait::Quadtree::getChild2Extent(e);
        REQUIRE(c.lo[0] == Approx(0.5));
        REQUIRE(c.lo[1] == Approx(0.5));
        REQUIRE(c.hi[0] == Approx(1.0));
        REQUIRE(c.hi[1] == Approx(1.0));
    }
    SECTION("Child 3") {
        Parfait::Extent<double> c = Parfait::Quadtree::getChild3Extent(e);
        REQUIRE(c.lo[0] == Approx(0.0));
        REQUIRE(c.lo[1] == Approx(0.5));
        REQUIRE(c.hi[0] == Approx(0.5));
        REQUIRE(c.hi[1] == Approx(1.0));
    }
}
