#include <RingAssertions.h>
#include <parfait/LagrangeTriangle.h>
#include <parfait/PointWriter.h>

inline double quadratic_surface(double x, double y) { return -x * x - y * y; }

TEST_CASE("Lagrange Triangle, linear") {
    Parfait::LagrangeTriangle::P2 tri = {
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.5, 0.0, 0.0}, {0.5, 0.5, 0.0}, {0.0, 0.5, 0.0}};

    {
        Parfait::Point<double> p = tri.evaluate(0.0, 0.0);
        REQUIRE(p[0] == Approx(0.0));
        REQUIRE(p[1] == Approx(0.0));
        REQUIRE(p[2] == Approx(0.0));
    }
    {
        Parfait::Point<double> p = tri.evaluate(1.0, 0.0);
        REQUIRE(p[0] == Approx(1.0));
        REQUIRE(p[1] == Approx(0.0));
        REQUIRE(p[2] == Approx(0.0));
    }
    {
        Parfait::Point<double> p = tri.evaluate(0.0, 1.0);
        REQUIRE(p[0] == Approx(0.0));
        REQUIRE(p[1] == Approx(1.0));
        REQUIRE(p[2] == Approx(0.0));
    }

    {
        Parfait::Point<double> p = tri.evaluate(0.5, 0.5);
        REQUIRE(p[0] == Approx(0.5));
        REQUIRE(p[1] == Approx(0.5));
        REQUIRE(p[2] == Approx(0.0));
    }

    {
        auto normal = tri.normal(0.0, 0.0);
        REQUIRE(normal[0] == Approx(0.0));
        REQUIRE(normal[1] == Approx(0.0));
        REQUIRE(normal[2] == Approx(1.0));
    }

    {
        auto normal = tri.normal(0.5, 0.5);
        REQUIRE(normal[0] == Approx(0.0));
        REQUIRE(normal[1] == Approx(0.0));
        REQUIRE(normal[2] == Approx(1.0));
    }

    {
        Parfait::Point<double> query_point = {0.2, 0.2, 1.0};
        auto closest = tri.closest(query_point);
        REQUIRE(closest[0] == Approx(0.2));
        REQUIRE(closest[1] == Approx(0.2));
        REQUIRE(closest[2] == Approx(0.0));
    }
}

TEST_CASE("Lagrange Triangle, sphere") {
    double a = std::sqrt(2.0) / 2.0;
    Parfait::LagrangeTriangle::P2 tri = {
        {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {a, a, 0.0}, {0.0, a, a}, {a, 0.0, a}};

    Parfait::Point<double> q = {2.0, 0.0, 0.0};

    auto points = tri.sample(100);
    Parfait::PointWriter::write("dog.3D", points);

    auto closest = tri.closest(q);
    REQUIRE(closest[0] == Approx(1.0));
    REQUIRE(closest[1] == Approx(0.0));
    REQUIRE(closest[2] == Approx(0.0));
}

TEST_CASE("Linear elements sides") {
    Parfait::LagrangeTriangle::P2 tri = {
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.5, 0.0, 0.0}, {0.5, 0.5, 0.0}, {0.0, 0.5, 0.0}};

    SECTION("Corner 0") {
        Parfait::Point<double> q = {-1.0, -1.0, 0.0};
        auto closest = tri.closest(q);
        CHECK(closest[0] == Approx(0.0));
        CHECK(closest[1] == Approx(0.0));
        CHECK(closest[2] == Approx(0.0));
    }

    SECTION("Corner 1") {
        Parfait::Point<double> q = {2.0, 0.0, 0.0};
        auto closest = tri.closest(q);
        CHECK(closest[0] == Approx(1.0));
        CHECK(closest[1] == Approx(0.0));
        CHECK(closest[2] == Approx(0.0));
    }

    SECTION("Corner 2") {
        Parfait::Point<double> q = {0.0, 2.0, 0.0};
        auto closest = tri.closest(q);
        CHECK(closest[0] == Approx(0.0));
        CHECK(closest[1] == Approx(1.0));
        CHECK(closest[2] == Approx(0.0));
    }

    SECTION("midpoint 0") {
        Parfait::Point<double> q = {0.5, -1.0, 0.0};
        auto closest = tri.closest(q);
        CHECK(closest[0] == Approx(0.5));
        CHECK(closest[1] == Approx(0.0));
        CHECK(closest[2] == Approx(0.0));
    }

    SECTION("midpoint 1") {
        Parfait::Point<double> q = {2.0, 2.0, 0.0};
        auto closest = tri.closest(q);
        double a = sqrt(2.0) / 2.0;
        CHECK(closest[0] == Approx(a));
        CHECK(closest[1] == Approx(a));
        CHECK(closest[2] == Approx(0.0));
    }

    SECTION("midpoint 2") {
        Parfait::Point<double> q = {-2.0, 0.5, 0.0};
        auto closest = tri.closest(q);
        CHECK(closest[0] == Approx(0.0));
        CHECK(closest[1] == Approx(0.5));
        CHECK(closest[2] == Approx(0.0));
    }
}

TEST_CASE("quadratic surface") {
    std::array<Parfait::Point<double>, 6> points = {Parfait::Point<double>{-1.0, -1.0, 0.0},
                                                    Parfait::Point<double>{1.0, -1.0, 0.0},
                                                    Parfait::Point<double>{0.0, 1.0, 0.0},
                                                    Parfait::Point<double>{0.0, -1.0, 0.0},
                                                    Parfait::Point<double>{0.5, 0.0, 0.0},
                                                    Parfait::Point<double>{-0.5, 0.0, 0.0}};

    Parfait::LagrangeTriangle::P2 tri = points;
    Parfait::Point<double> query(0.0, 0.0, 1.0);

    for (auto& p : points) {
        p[2] = quadratic_surface(p[0], p[1]);
    }

    SECTION("verify newton line search") {
        REQUIRE(tri.lineSearchScaling(query, 1.0, 1.0, 0.25, 0.5) == 0.0);
        REQUIRE(tri.lineSearchScaling(query, -0.5, -0.75, 0.0, 0.0) == 0.5);
        REQUIRE(tri.lineSearchScaling(query, -0.25, -0.5, 0.0, 0.0) == 1.0);
    }
    SECTION("solve") {
        auto c = tri.closest(query);
        REQUIRE(c[0] == Approx(0.0).margin(1.e-8));
        REQUIRE(c[1] == Approx(0.0).margin(1.e-8));
        REQUIRE(c[2] == Approx(0.0).margin(1.e-8));
    }
}
