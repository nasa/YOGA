#include <parfait/CartBlock.h>
#include <parfait/Extent.h>
#include <parfait/MarchingCubes.h>
#include <RingAssertions.h>
#include <vector>

TEST_CASE("Marching cubes on a single extent box") {
    Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}};

    std::array<double, 8> scalar = {-1, 1, 1, 1, 1, 1, 1, 1};

    auto facets = Parfait::MarchingCubes::marchingCubes(e, scalar.data());
    REQUIRE(facets.size() == 1);
}

TEST_CASE("Marching cubes on mesh with sphere") {
    auto signedDistance = [](const Parfait::Point<double>& p) {
        auto mag = p.magnitude();
        return 0.8 - mag;  // sphere radius 8
    };

    auto mesh = Parfait::CartBlock({-2, -2, -2}, {2, 2, 2}, 25, 25, 25);
    int facet_count = 0;
    for (int c = 0; c < mesh.numberOfCells(); c++) {
        auto e = mesh.createExtentFromCell(c);
        std::array<double, 8> scalar;
        auto corners = e.corners();
        for (int i = 0; i < 8; i++) {
            scalar[i] = signedDistance(corners[i]);
        }
        auto new_facets = Parfait::MarchingCubes::marchingCubes(e, scalar.data());
        facet_count += new_facets.size();
        //        stl.facets.insert(stl.facets.end(), new_facets.begin(), new_facets.end());
    }
    REQUIRE(facet_count == 956);
    //    stl.writeAsciiFile("marching-cubes");
}
