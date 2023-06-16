#include <RingAssertions.h>
#include <parfait/Point.h>
#include <parfait/PointGenerator.h>
#include <parfait/PointWriter.h>
#include <parfait/Linspace.h>
#include <t-infinity/Geometry.h>


inline Parfait::Point<double> circleEvaluate(double t,
                                             Parfait::Point<double> centroid = {0, 0, 0},
                                             double radius = 1.0) {
    Parfait::Point<double> p;
    t = t * M_PI;
    p[0] = cos(t);
    p[1] = sin(t);
    p[2] = 0.0;
    return p * radius + centroid;
}


TEST_CASE("project onto single edge segment") {
    inf::geom_impl::PiecewiseP1Edge geometry({{0, 0, 0}, {0.5, 0.5, 0.0}, {1, 1, 0}});
    auto left = geometry.points[0];
    auto right = geometry.points[2];
    auto center = 0.5*(left + right);
    auto v = right - left;
    v.normalize();

    Parfait::Point<double> l_point = {-1,0,0};
    Parfait::Point<double> r_point = {1,1,0};
    Parfait::Point<double> c_point = {0.5, 0.5, 0.1};

    REQUIRE(geometry.project(l_point).approxEqual(left));
    REQUIRE(geometry.project(c_point).approxEqual(center));
    REQUIRE(geometry.project(r_point).approxEqual(right));
}

TEST_CASE("Geometry exists", "[oct-circle-test]") {
    std::vector<Parfait::Point<double>> points;
    auto t_values = Parfait::linspace(0.0, 1.0, 100);
    for (auto t : t_values) points.push_back(circleEvaluate(t));

    REQUIRE(points.front()[0] == Approx(1.0));
    REQUIRE(points.front()[1] == Approx(0.0).margin(1e-8));
    REQUIRE(points.back()[0] == Approx(-1.0));
    REQUIRE(points.back()[1] == Approx(0.0).margin(1e-8));

    inf::geom_impl::PiecewiseP1Edge half_circle(points);

    {
        auto p = Parfait::Point<double>{-1, 0.0, 0.0};
        auto z = half_circle.project(p);
        REQUIRE(z[0] == Approx(p[0]).margin(1e-8));
        REQUIRE(z[1] == Approx(p[1]).margin(1e-8));
    }

    {
        auto p = Parfait::Point<double>{1, 0.0, 0.0};
        auto z = half_circle.project(p);
        REQUIRE(z[0] == Approx(p[0]).margin(1e-8));
        REQUIRE(z[1] == Approx(p[1]).margin(1e-8));
    }
}
