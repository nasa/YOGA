#include <parfait/CartBlock.h>
#include <parfait/Extent.h>
#include <parfait/ExtentBuilder.h>
#include <RingAssertions.h>
#include <vector>

Parfait::Extent<double> getIntersectingExtent(const std::vector<Parfait::Extent<double>>& extents) {
    std::vector<Parfait::Extent<double>> intersections;
    for (size_t i = 0; i < extents.size(); i++) {
        auto& a = extents[i];
        for (size_t j = 0; j < extents.size(); j++) {
            if (i != j) {
                auto& b = extents[j];
                if (a.intersects(b)) intersections.emplace_back(a.intersection(b));
            }
        }
    }
    auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    for (auto& intersection : intersections) {
        Parfait::ExtentBuilder::add(e, intersection.lo);
        Parfait::ExtentBuilder::add(e, intersection.hi);
    }
    return e;
}

Parfait::CartBlock generateCartBlock(const Parfait::Extent<double>& e, int max_cells) {
    double dxdy = e.getLength_X() / e.getLength_Y();
    double dxdz = e.getLength_X() / e.getLength_Z();
    int nx, ny, nz;
    nx = std::max(1, int(std::cbrt(max_cells)));
    ny = std::max(1, int(nx / dxdy));
    nz = std::max(1, int(nx / dxdz));
    while (nx * ny * nz <= max_cells) {
        nx++;
        ny = std::max(1, int(nx / dxdy));
        nz = std::max(1, int(nx / dxdz));
    }
    nx--;
    ny = std::max(1, int(nx / dxdy));
    nz = std::max(1, int(nx / dxdz));
    return {e, nx, ny, nz};
}

TEST_CASE("determine extent box of intersection of collection of extents") {
    std::vector<Parfait::Extent<double>> extents;
    extents.push_back({{0, 0, 0}, {100, 100, 100}});
    extents.push_back({{5, 3, 2}, {6, 4, 3}});

    auto e = getIntersectingExtent(extents);

    REQUIRE(extents[1].approxEqual(e));

    extents.push_back({{9, 9, 9}, {11, 12, 13}});
    e = getIntersectingExtent(extents);
    REQUIRE(5 == e.lo[0]);
    REQUIRE(3 == e.lo[1]);
    REQUIRE(2 == e.lo[2]);
    REQUIRE(11 == e.hi[0]);
    REQUIRE(12 == e.hi[1]);
    REQUIRE(13 == e.hi[2]);
}

TEST_CASE("Determine dimensions of cart block with N cells, close to isotropic") {
    SECTION("Create isotropic block") {
        Parfait::Extent<double> e{{0, 0, 0}, {9, 9, 9}};
        int max_cells = 1000000;
        auto block = generateCartBlock(e, max_cells);

        REQUIRE(block.approxEqual(e));
        REQUIRE(100 == block.numberOfCells_X());
        REQUIRE(100 == block.numberOfCells_Y());
        REQUIRE(100 == block.numberOfCells_Z());
    }

    SECTION("Try to make isotropic cells in non-isotropic block") {
        Parfait::Extent<double> e{{0, 0, 0}, {9, 2, 1}};
        const int max_cells = 1000000;
        auto block = generateCartBlock(e, max_cells);

        REQUIRE(block.approxEqual(e));
        REQUIRE(999248 == block.numberOfCells());
        REQUIRE(346 == block.numberOfCells_X());
        REQUIRE(76 == block.numberOfCells_Y());
        REQUIRE(38 == block.numberOfCells_Z());
    }
}
