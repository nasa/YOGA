#include <parfait/Point.h>
#include <parfait/PointWriter.h>
#include <parfait/STLFactory.h>
#include <parfait/StitchMesh.h>
#include <parfait/TaubinSmoothing.h>
#include <parfait/LaplacianSmoothing.h>
#include <RingAssertions.h>

TEST_CASE("Taubin Smoothing Exists") {
    double radius = 0.8;
    double pi = 4.0 * atan(1.0);
    auto field = [=](const Parfait::Point<double>& p) {
        auto r = p.magnitude();
        double phi = atan(p[1] / p[0]);
        double theta = acos(p[2] / r);
        double f = radius - r;
        if (p[0] > 0 and p[1] > 0) f += 0.1 * (cos(2. * phi * pi) + sin(2 * theta * pi));
        return f;
    };
    auto sphere_facets = Parfait::STLFactory::generateSurface(field, {{-2, -2, -2}, {2, 2, 2}}, 50, 50, 50);

    std::vector<int> facet_nodes;
    std::vector<Parfait::Point<double>> points;
    auto get_facet = [&](int i) { return sphere_facets[i]; };
    std::tie(facet_nodes, points) = Parfait::stitchFacets(get_facet, sphere_facets.size());
    auto original_points = points;

    SECTION("smooth all points") {
        Parfait::TaubinSmoothing::taubinSmoothing(facet_nodes, points, 10);
        sphere_facets = Parfait::STL::convertToFacets(facet_nodes, points);
        for (size_t n = 0; n < original_points.size(); n++) {
            REQUIRE(original_points[n] != points[n]);
        }
    }

    SECTION("smooth no points") {
        std::vector<double> damping_weights(points.size(), 0.0);
        Parfait::TaubinSmoothing::taubinSmoothing(facet_nodes, points, damping_weights);
        for (size_t n = 0; n < original_points.size(); n++) {
            REQUIRE(original_points[n] == points[n]);
        }
    }
}

TEST_CASE("Laplacian Smoothing") {
    std::vector<std::vector<int>> n2n;
    n2n.push_back({1, 2});
    n2n.push_back({0});
    n2n.push_back({0, 1, 2});
    std::vector<double> field = {1, 2, 3};
    Parfait::laplacianSmoothing(0.0, n2n, field);
    REQUIRE(2.5 == Approx(field[0]));
    REQUIRE(1.0 == Approx(field[1]));
    REQUIRE(2.0 == Approx(field[2]));
}