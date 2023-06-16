#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/SMesh.h>
#include <t-infinity/StructuredMeshSequencing.h>

TEST_CASE("Can coarsen a structured mesh") {
    inf::SMesh mesh;
    using Point = Parfait::Point<double>;
    using Dimensions = std::array<int, 3>;
    SECTION("Invalid coarsening factor throws") {
        auto block = inf::CartesianBlock({0, 0, 0}, {1, 1, 1}, {16, 16, 16});
        REQUIRE_NOTHROW(inf::coarsenStructuredBlock(block, {2, 2, 2}));
        REQUIRE_THROWS(inf::coarsenStructuredBlock(block, {2, 2, 5}));
    }
    SECTION("Coarsen mesh") {
        mesh.add(std::make_shared<inf::CartesianBlock>(Point{0, 0, 0}, Point{1, 1, 1}, Dimensions{16, 16, 16}, 0));
        mesh.add(std::make_shared<inf::CartesianBlock>(Point{1, 0, 0}, Point{2, 1, 1}, Dimensions{8, 4, 32}, 1));
        auto coarse_mesh = inf::coarsenStructuredMesh(mesh, {2, 2, 4});

        std::vector<Dimensions> expected_dimensions = {{9, 9, 5}, {5, 3, 9}};
        for (int block_id : mesh.blockIds()) {
            const auto& block = coarse_mesh->getBlock(block_id);
            auto dimensions = block.nodeDimensions();
            REQUIRE(expected_dimensions[block_id] == dimensions);
            for (int i = 0; i < dimensions[0]; ++i) {
                for (int j = 0; j < dimensions[1]; ++j) {
                    for (int k = 0; k < dimensions[2]; ++k) {
                        auto expected = mesh.getBlock(block_id).point(i * 2, j * 2, k * 4);
                        auto actual = block.point(i, j, k);
                        INFO("block: " << block_id << " i: " << i << " j: " << j << " k: " << k);
                        INFO("expected: " << expected.to_string() << " actual: " << actual.to_string());
                        REQUIRE(expected.approxEqual(actual));
                    }
                }
            }
        }
    }
}

TEST_CASE("Can double a StructuredBlock") {
    auto block = inf::CartesianBlock({0, 0, 0}, {1, 1, 1}, {16, 16, 16});
    auto doubled = inf::doubleStructuredBlock(block, inf::K);
    REQUIRE(block.nodeDimensions()[0] == doubled->nodeDimensions()[0]);
    REQUIRE(block.nodeDimensions()[1] == doubled->nodeDimensions()[1]);
    REQUIRE(2 * block.nodeDimensions()[2] - 1 == doubled->nodeDimensions()[2]);

    inf::loopMDRange({0, 0, 0}, block.nodeDimensions(), [&](int i, int j, int k) {
        {
            auto expected = block.point(i, j, k);
            auto actual = doubled->point(i, j, k * 2);
            REQUIRE(expected.approxEqual(actual));
        }
        if (k != block.nodeDimensions()[2] - 1) {
            auto expected = 0.5 * (block.point(i, j, k) + block.point(i, j, k + 1));
            auto actual = doubled->point(i, j, k * 2 + 1);
            INFO(i << j << k);
            INFO("expected: " << expected.to_string() << " actual: " << actual.to_string());
            REQUIRE(expected.approxEqual(actual));
        }
    });
}