#include <RingAssertions.h>
#include <parfait/Point.h>
#include <parfait/EdgeSegmentProjection.h>
using namespace Parfait;

TEST_CASE("Edge segment projection") {
    Point<double> start = {0, 0, 0};
    Point<double> end = {2, 0, 0};

    SECTION("Point is on the left") {
        Point<double> query = {-1, 0, 0};
        auto p = closestPointOnEdge(start, end, query);
        REQUIRE(p == start);
    }
    SECTION("Point is on the right") {
        Point<double> query = {3, 0, 0};
        auto p = closestPointOnEdge(start, end, query);
        REQUIRE(p == end);
    }
    SECTION("Point is in the middle") {
        Point<double> query = {0.3, .1, 0};
        auto p = closestPointOnEdge(start, end, query);
        REQUIRE(p == Point<double>{0.3, 0, 0});
    }
}

TEST_CASE("Zero length edge segments") {
    Point<double> start = {0, 1, 0};
    Point<double> end = {0, 1, 0};

    SECTION("Beyond the end") {
        Point<double> query = {4, 4, 4};
        auto p = closestPointOnEdge(start, end, query);
        REQUIRE(p == Point<double>{0, 1, 0});
    }
    SECTION("Before the beginning") {
        Point<double> query = {0, 0, 0};
        auto p = closestPointOnEdge(start, end, query);
        REQUIRE(p == Point<double>{0, 1, 0});
    }
    SECTION("on the left") {
        Point<double> query = {0, 0.5, 0};
        auto p = closestPointOnEdge(start, end, query);
        REQUIRE(p == Point<double>{0, 1, 0});
    }
    SECTION("on the right") {
        Point<double> query = {0.75, 0.5, 0};
        auto p = closestPointOnEdge(start, end, query);
        REQUIRE(p == Point<double>{0, 1, 0});
    }
}

TEST_CASE("extracted from failing distance calculation") {
    Point<double> a = {6.758159e-01, 4.046650e-01, 0.000000e+00};
    Point<double> b = {6.764120e-01, 4.057727e-01, 0.000000e+00};
    Point<double> q = {9.900000e-01, 9.900000e-01, 0.000000e+00};

    auto p = closestPointOnEdge(a, b, q);
    REQUIRE(p.distance(a) <= a.distance(b));
}
