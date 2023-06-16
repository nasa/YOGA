#include <RingAssertions.h>
#include <stdio.h>
#include <vector>
#include <array>
#include <parfait/Point.h>
#include <parfait/STLFactory.h>
#include <parfait/StitchMesh.h>
#include <parfait/StringTools.h>
#include <parfait/Throw.h>
#include <parfait/TetGenWriter.h>
#include <parfait/Flatten.h>

TEST_CASE("Tetgen smesh and poly Writer.  Example from tetgen distribution") {
    std::vector<Parfait::Point<double>> points = {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};

    std::vector<std::array<int, 4>> quads = {
        {0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 6, 7}, {3, 0, 4, 7}};
    std::vector<std::array<int, 3>> tris = {};
    std::vector<int> tri_tags = {};
    std::vector<int> quad_tags(quads.size(), 0);

    int num_points = points.size();
    auto getPoint = [&](int i) { return points[i]; };

    int num_tris = tris.size();
    auto fillTri = [&](int t, long* tri) {
        tri[0] = tris[t][0];
        tri[1] = tris[t][1];
        tri[2] = tris[t][2];
    };

    auto getTriTag = [](int t) { return 0; };

    int num_quads = 0;
    auto fillQuad = [&](int t, long* q) {};
    auto getQuadTag = [](int t) { return 0; };

    Parfait::TetGen::writeSmesh(
        "example.smesh", getPoint, num_points, fillTri, getTriTag, num_tris, fillQuad, getQuadTag, num_quads);
}
TEST_CASE("Write sphere to tetgen") {
    auto facets = Parfait::STLFactory::generateSphere({0, 0, 0});
    std::vector<int> tris;
    std::vector<Parfait::Point<double>> points;
    std::tie(tris, points) = Parfait::stitchFacets(facets);
    int num_tris = tris.size() / 3;
    std::vector<int> tri_tags(num_tris, 0);

    auto fillTri = [&](int t, long* tri) {
        tri[0] = tris[3 * t + 0];
        tri[1] = tris[3 * t + 1];
        tri[2] = tris[3 * t + 2];
    };

    auto getPoint = [&](int i) { return points[i]; };
    auto getTriTag = [](int t) { return 0; };

    int num_quads = 0;
    auto fillQuad = [&](int t, long* q) {};
    auto getQuadTag = [](int t) { return 0; };

    Parfait::TetGen::writeSmesh(
        "sphere", getPoint, points.size(), fillTri, getTriTag, num_tris, fillQuad, getQuadTag, num_quads);
    Parfait::TetGen::writePoly(
        "example.poly", getPoint, points.size(), fillTri, getTriTag, num_tris, fillQuad, getQuadTag, num_quads);
}

TEST_CASE("Write sphere in sphere with holes") {
    auto outer_sphere = Parfait::STLFactory::generateSphere({0, 0, 0}, 10);
    auto inner_sphere = Parfait::STLFactory::generateSphere({0, 0, 0}, 1);
    std::vector<Parfait::Facet> facets =
        Parfait::flatten(std::vector<std::vector<Parfait::Facet>>{outer_sphere, inner_sphere});
    std::vector<int> tris;
    std::vector<Parfait::Point<double>> points;
    std::tie(tris, points) = Parfait::stitchFacets(facets);
    int num_tris = tris.size() / 3;
    std::vector<int> tri_tags(num_tris, 0);

    auto fillTri = [&](int t, long* tri) {
        tri[0] = tris[3 * t + 0];
        tri[1] = tris[3 * t + 1];
        tri[2] = tris[3 * t + 2];
    };

    auto getPoint = [&](int i) { return points[i]; };
    auto getTriTag = [](int t) { return 0; };

    int num_quads = 0;
    auto fillQuad = [&](int t, long* q) {};
    auto getQuadTag = [](int t) { return 0; };

    Parfait::Point<double> hole_point = {0, 0, 0};
    int num_hole_points = 1;
    auto getHolePoint = [&](int i) { return hole_point; };

    Parfait::TetGen::writePoly("example.poly",
                               getPoint,
                               points.size(),
                               fillTri,
                               getTriTag,
                               num_tris,
                               fillQuad,
                               getQuadTag,
                               num_quads,
                               num_hole_points,
                               getHolePoint);
}
