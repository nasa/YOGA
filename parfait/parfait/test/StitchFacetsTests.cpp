#include <parfait/StitchMesh.h>
#include <RingAssertions.h>

TEST_CASE("Stitch a single facet") {
    std::array<Parfait::Point<double>, 3> f;
    f[0] = Parfait::Point<double>{0, 0, 0};
    f[1] = Parfait::Point<double>{1, 0, 0};
    f[2] = Parfait::Point<double>{0, 1, 0};

    auto getFacet = [&](int i) { return f; };

    std::vector<int> tri;
    std::vector<Parfait::Point<double>> points;
    std::tie(tri, points) = Parfait::stitchFacets(getFacet, 1);

    REQUIRE(tri.size() == 3);
    REQUIRE(points.size() == 3);
}

TEST_CASE("Don't add degenerate triangles") {
    std::vector<Parfait::Facet> facets;
    Parfait::Facet f{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    facets.push_back(f);
    f = Parfait::Facet{{0, 0, 0}, {1, 0, 0}, {1, 0, 0}};
    facets.push_back(f);

    auto getFacet = [&](int i) { return facets[i]; };

    std::vector<int> tri;
    std::vector<Parfait::Point<double>> points;
    std::tie(tri, points) = Parfait::stitchFacets(getFacet, 2);

    REQUIRE(tri.size() == 3);
    REQUIRE(points.size() == 3);
}
