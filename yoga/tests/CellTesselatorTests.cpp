#include <parfait/Point.h>
#include <RingAssertions.h>
#include <parfait/CellTesselator.h>

double computeTetVolume(const Parfait::CellTesselator::Tet& tet) {
    auto a = tet[0];
    auto b = tet[1];
    auto c = tet[2];
    auto d = tet[3];
    auto v1 = a - d;
    auto v2 = b - d;
    auto v3 = c - d;
    auto v = Parfait::Point<double>::cross(v2, v3);
    return -Parfait::Point<double>::dot(v1, v) / 6.0;
}

TEST_CASE("tesselate pyramid") {
    Parfait::CellTesselator::Pyramid pyramid;
    pyramid[0] = {0, 0, 0};
    pyramid[1] = {4, 0, 0};
    pyramid[2] = {4, 4, 0};
    pyramid[3] = {0, 4, 0};
    pyramid[4] = {2, 2, 5};

    SECTION("conformal tesselation, using quad face centroid for splitting") {
        auto tets = Parfait::CellTesselator::tesselate(pyramid);

        REQUIRE(4 == tets.size());
        double total_volume = 0.0;
        for (auto tet : tets) total_volume += computeTetVolume(tet);

        double pyramid_volume = (4 * 4 * 5) / 3.0;
        REQUIRE(pyramid_volume == Approx(total_volume));
    }

    SECTION("generate overlapping tets, based on splitting the quad face both ways"){
        auto tets = Parfait::CellTesselator::tesselateNonConformally(pyramid);
        REQUIRE(4 == tets.size());
        double total_volume = 0.0;
        for (auto tet : tets) total_volume += computeTetVolume(tet);

        double actual_pyramid_volume = (4 * 4 * 5) / 3.0;
        REQUIRE(actual_pyramid_volume == Approx(0.5 * total_volume));
    }
}

TEST_CASE("tessalate prism") {
    Parfait::CellTesselator::Prism prism;
    prism[0] = {0, 0, 0};
    prism[1] = {1, 0, 0};
    prism[2] = {0, 1, 0};
    prism[3] = {0, 0, 5};
    prism[4] = {1, 0, 5};
    prism[5] = {0, 1, 5};

    SECTION("Tesselate by adding steiner point at each quad centroid"){
        auto tets = Parfait::CellTesselator::tesselate(prism);

        double prism_volume = 0.5 * 5;

        double total_volume = 0.0;
        for (auto tet : tets) total_volume += computeTetVolume(tet);

        REQUIRE(prism_volume == Approx(total_volume));
    }

    SECTION("Generate tets for every combination of split quad faces"){
        auto tets = Parfait::CellTesselator::tesselateNonConformally(prism);

        double prism_volume = 0.5 * 5;

        double total_volume = 0.0;
        for (auto tet : tets) {
            total_volume += computeTetVolume(tet);
        }
        // the only volume not doubled is that of the triangle faces, so add them here
        total_volume += computeTetVolume(tets[0]);
        total_volume += computeTetVolume(tets[1]);

        REQUIRE(prism_volume == Approx(0.5*total_volume));
    }

}

TEST_CASE("tesselate hexahedron") {
    Parfait::CellTesselator::Hex hex;
    hex[0] = {0, 0, 0};
    hex[1] = {1, 0, 0};
    hex[2] = {1, 1, 0};
    hex[3] = {0, 1, 0};
    hex[4] = {0, 0, 7};
    hex[5] = {1, 0, 7};
    hex[6] = {1, 1, 7};
    hex[7] = {0, 1, 7};

    SECTION("Tesselate by adding steiner point to each quad face") {
        auto tets = Parfait::CellTesselator::tesselate(hex);

        double hex_volume = 7.0;

        double total_volume = 0.0;
        for (auto tet : tets) total_volume += computeTetVolume(tet);

        REQUIRE(hex_volume == Approx(total_volume));
    }

    SECTION("Tesselate by making tets with every possible quad face diagonal split") {
        auto tets = Parfait::CellTesselator::tesselateNonConformally(hex);

        double hex_volume = 7.0;

        double total_volume = 0.0;
        for (auto tet : tets) total_volume += computeTetVolume(tet);

        REQUIRE(hex_volume == Approx(0.5*total_volume));
    }
}
