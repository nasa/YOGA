#include <parfait/Intersections.h>
#include <RingAssertions.h>

using namespace Parfait::Intersections;

TEST_CASE("Edge plane intersection ") {
    Parfait::Point<double> normal = {1, 0, 0};
    Parfait::Point<double> some_point_on_the_plane = {0.1, 0, 0};

    Parfait::Point<double> a = {0, 0, 0};
    Parfait::Point<double> b = {1, 0, 0};

    double weight = edgeAndPlane(a, b, some_point_on_the_plane, normal);
    REQUIRE(weight == Approx(0.9));
    auto intersection = weight * a + (1.0 - weight) * b;
    REQUIRE(intersection[0] == Approx(0.1));
}

TEST_CASE("Edge plane intersection not on unit edge") {
    Parfait::Point<double> normal = {1, 0, 0};
    Parfait::Point<double> some_point_on_the_plane = {0.2, 0, 0};

    Parfait::Point<double> a = {0, 0, 0};
    Parfait::Point<double> b = {2, 0, 0};

    double weight = edgeAndPlane(a, b, some_point_on_the_plane, normal);
    REQUIRE(weight == Approx(0.9));
    auto intersection = weight * a + (1.0 - weight) * b;
    REQUIRE(intersection[0] == Approx(0.2));
}

TEST_CASE("Plane and edge don't intersect") {
    Parfait::Point<double> normal = {1, 0, 0};
    Parfait::Point<double> some_point_on_the_plane = {-0.2, 0, 0};

    Parfait::Point<double> a = {0, 0, 0};
    Parfait::Point<double> b = {2, 0, 0};

    double weight = edgeAndPlane(a, b, some_point_on_the_plane, normal);

    auto intersection = weight * a + (1.0 - weight) * b;
    REQUIRE(intersection[0] == Approx(-0.2));
    REQUIRE(weight > 1.0);
}

TEST_CASE("Triangle and ray intersection") {
    Parfait::Point<double> a = {0, 0, 1};
    Parfait::Point<double> b = {1, 0, 1};
    Parfait::Point<double> c = {0, 1, 1};

    Parfait::Point<double> origin = {.1, .1, -.1};
    Parfait::Point<double> normal = {0, 0, 1};

    auto weights = triangleAndRay(a, b, c, origin, normal);
    REQUIRE(weights[0] >= 0.0);
    REQUIRE(weights[0] <= 1.0);
    REQUIRE(weights[1] >= 0.0);
    REQUIRE(weights[1] <= 1.0);
    REQUIRE(weights[2] >= 0.0);
    REQUIRE(weights[2] <= 1.0);
    auto p = weights[0] * a + weights[1] * b + weights[2] * c;
    REQUIRE(p[0] == Approx(0.1));
    REQUIRE(p[1] == Approx(0.1));
    REQUIRE(p[2] == Approx(1.0));
}