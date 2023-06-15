#include <parfait/Extent.h>
#include <parfait/Facet.h>
#include <parfait/Plane.h>
#include <parfait/PolygonClipping.h>
#include <parfait/STL.h>

#include <RingAssertions.h>

using namespace Parfait::Polygons;

TEST_CASE("Line segment plane intersection") {
    Parfait::Point<double> p;
    auto d = planeClipping(Parfait::Plane<double>{{1, 0, 0}, {.5, 0, 0}}, {0, 0, 0}, {1, 0, 0}, p);
    REQUIRE(d);
    REQUIRE(p[0] == Approx(0.5));
}
TEST_CASE("Line segment wont intersect out of bounds") {
    Parfait::Point<double> p;
    auto d = planeClipping(Parfait::Plane<double>{{1, 0, 0}, {.5, 0, 0}}, {2, 0, 0}, {3, 0, 0}, p);
    REQUIRE_FALSE(d);
}

TEST_CASE("Triangle plane intersection") {
    Polygon polygon = {{.25, .25, .0}, {.25, -.25, 0}, {.75, -.25, 0}};
    SECTION("Y plus") {
        polygon = planeClipping(Parfait::Plane<double>{{0, 1, 0}, {0, 0, 0}}, polygon);
        REQUIRE(polygon.size() == 3);
    }
    SECTION("Y minus") {
        polygon = planeClipping(Parfait::Plane<double>{{0, -1, 0}, {0, 0, 0}}, polygon);
        REQUIRE(polygon.size() == 4);
    }
}

TEST_CASE("Triangle AABB intersection") {
    Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}};
    Parfait::Facet f;
    f[0] = {.5, .5, .2};
    f[1] = {.5, 2, .2};
    f[2] = {2, 1.5, .2};

    auto polygon = intersection(e, f);
    CHECK(polygon.size() == 4);
    auto tris = triangulate(polygon);
    CHECK(tris.size() == 2);
}

TEST_CASE("Triangulate a triangle into one triangle") {
    Polygon p;
    p.push_back({0, 0, 0});
    p.push_back({1, 0, 0});
    p.push_back({0, 1, 0});

    auto tris = triangulate(p);
    REQUIRE(tris.size() == 1);
}

TEST_CASE("in front") {
    Parfait::Plane<double> plane = {{1, 0, 0}, {.5, .5, .5}};
    REQUIRE_FALSE(inFront(plane, {0, 0, 0}));
}

TEST_CASE("Polygon plane intersection") {
    Parfait::Plane<double> plane = {{-1, 0, 0}, {.5, .5, .5}};
    Polygon polygon;
    polygon.emplace_back(Parfait::Point<double>{0, 0, 0});
    polygon.emplace_back(Parfait::Point<double>{1, 0, 0});
    polygon.emplace_back(Parfait::Point<double>{0, 1, 0});

    auto clipped = planeClipping(plane, polygon);
    CHECK(clipped.size() == 4);
}

TEST_CASE("Triangulate a quad into two triangles") {
    Polygon p;
    p.push_back({0, 0, 0});
    p.push_back({1, 0, 0});
    p.push_back({1, 1, 0});
    p.push_back({0, 1, 0});

    auto tris = triangulate(p);
    REQUIRE(tris.size() == 2);
}

TEST_CASE("Get Extent Box as polygons") {
    Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}};
    auto sides = extentAsPolyhedron(e);
    REQUIRE(sides.size() == 6);
    REQUIRE(sides[0].size() == 4);

    auto tris = triangulate(sides);
    REQUIRE(tris.size() == 12);
}

TEST_CASE("Extent box surface intersection") {
    Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}};
    auto sides = extentAsPolyhedron(e);

    Parfait::Plane<double> plane = {{1, 0, 0}, {.5, .5, .5}};
    auto polygons = planeClipping(plane, sides);
}
