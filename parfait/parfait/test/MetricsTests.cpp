#include <RingAssertions.h>
#include <parfait/Point.h>
#include <parfait/Metrics.h>

TEST_CASE("right tet volume") {
    Parfait::Point<double> p1 = {0, 0, 0};
    Parfait::Point<double> p2 = {1, 0, 0};
    Parfait::Point<double> p3 = {0, 1, 0};
    Parfait::Point<double> p4 = {0, 0, 1};
    auto v = Parfait::Metrics::computeTetVolume(p1, p2, p3, p4);
    REQUIRE(v == Approx(1.0 / 6.0));
}

TEST_CASE("Bad tet volume") {
    Parfait::Point<double> p1 = {21.933749, 5.277811, 1.537161};
    Parfait::Point<double> p2 = {21.933052, 5.276541, 1.537878};
    Parfait::Point<double> p3 = {21.932057, 5.277119, 1.537174};
    Parfait::Point<double> p4 = {21.931910, 5.276684, 1.537631};

    auto v = Parfait::Metrics::computeTetVolume(p1, p2, p3, p4);
    REQUIRE(v < 0);
}

TEST_CASE("Quad area vectors") {
    Parfait::Point<double> a = {0, 0, 0};
    Parfait::Point<double> b = {1, 0, 0};
    Parfait::Point<double> c = {1, 1, 0};
    Parfait::Point<double> d = {0, 1, 0};

    auto quad_corner_areas = Parfait::Metrics::calcCornerAreas(std::array<Parfait::Point<double>, 4>{a, b, c, d});
    for (int i = 0; i < 4; i++) {
        REQUIRE(quad_corner_areas[i][0] == Approx(0.0));
        REQUIRE(quad_corner_areas[i][1] == Approx(0.0));
        REQUIRE(quad_corner_areas[i][2] == Approx(0.25));
    }
}

TEST_CASE("Tri area vectors") {
    Parfait::Point<double> a = {0, 0, 0};
    Parfait::Point<double> b = {1, 0, 0};
    Parfait::Point<double> c = {0, 1, 0};

    auto tri_corner_areas = Parfait::Metrics::calcCornerAreas(std::array<Parfait::Point<double>, 3>{a, b, c});
    double sum = 0.0;
    for (int i = 0; i < 3; i++) {
        sum += tri_corner_areas[i][2];
    }
    REQUIRE(sum == Approx(0.5));
}

TEST_CASE("XY line normal") {
    Parfait::Point<double> a = {0.3, 0.4, 2.0};
    Parfait::Point<double> b = {1.0, 2.7, 2.0};
    Parfait::Point<double> expected(b[1] - a[1], a[0] - b[0], 0);
    auto actual = Parfait::Metrics::XYLineNormal(a, b);
    INFO("expected: " << actual.to_string() << " expected: " << expected.to_string());
    REQUIRE(expected.approxEqual(actual));
    REQUIRE((b - a).magnitude() == Approx(actual.magnitude()));
}
