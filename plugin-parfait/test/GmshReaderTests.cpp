#include <parfait/ExtentBuilder.h>
#include <RingAssertions.h>
#include "GmshReader.h"

std::string grid_file = std::string(GRIDS_DIR) + "gmsh/mixed_element_cube.msh";

constexpr int ExpectedNodeCount = 13396;
constexpr int ExpectedTriCount = 528;
constexpr int ExpectedQuadCount = 126;
constexpr int ExpectedTetCount = 15953;
constexpr int ExpectedPyramidCount = 791;
constexpr int ExpectedPrismCount = 7665;
constexpr int ExpectedHexCount = 5521;

using namespace inf;

TEST_CASE("Get metadata") {
    GmshReader reader(grid_file);
    REQUIRE(ExpectedNodeCount == reader.nodeCount());
    REQUIRE(ExpectedTriCount == reader.cellCount(MeshInterface::TRI_3));
    REQUIRE(ExpectedQuadCount == reader.cellCount(MeshInterface::QUAD_4));
    REQUIRE(ExpectedTetCount == reader.cellCount(MeshInterface::TETRA_4));
    REQUIRE(ExpectedPyramidCount == reader.cellCount(MeshInterface::PYRA_5));
    REQUIRE(ExpectedPrismCount == reader.cellCount(MeshInterface::PENTA_6));
    REQUIRE(ExpectedHexCount == reader.cellCount(MeshInterface::HEXA_8));
}

TEST_CASE("read nodes") {
    GmshReader reader(grid_file);
    auto nodes = reader.readCoords();
    REQUIRE(nodes.size() == ExpectedNodeCount);
    SECTION("Read in two slices, and verify same result") {
        auto first_chunk = reader.readCoords(0, 257);
        auto second_chunk = reader.readCoords(257, ExpectedNodeCount);
        auto all_nodes = first_chunk;
        all_nodes.insert(all_nodes.end(), second_chunk.begin(), second_chunk.end());
        REQUIRE(ExpectedNodeCount == all_nodes.size());
        auto extent = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        for (size_t i = 0; i < all_nodes.size(); i++) {
            REQUIRE(all_nodes[i].approxEqual(nodes[i]));
            Parfait::ExtentBuilder::add(extent, nodes[i]);
        }
        REQUIRE(8.0 == Approx(extent.volume(extent)));
    }
}

TEST_CASE("Read cells") {
    GmshReader reader(grid_file);
    auto tets = reader.readCells(MeshInterface::TETRA_4);
    REQUIRE(4 * ExpectedTetCount == tets.size());
    auto tris = reader.readCells(MeshInterface::TRI_3);
    REQUIRE(3 * ExpectedTriCount == tris.size());
    auto quads = reader.readCells(MeshInterface::QUAD_4);
    REQUIRE(4 * ExpectedQuadCount == quads.size());
    auto pyramids = reader.readCells(MeshInterface::PYRA_5);
    REQUIRE(5 * ExpectedPyramidCount == pyramids.size());
    auto prisms = reader.readCells(MeshInterface::PENTA_6);
    REQUIRE(6 * ExpectedPrismCount == prisms.size());
    auto hexs = reader.readCells(MeshInterface::HEXA_8);
    REQUIRE(8 * ExpectedHexCount == hexs.size());

    SECTION("Read in chunks and verify same result") {
        auto chunk1 = reader.readCells(MeshInterface::TETRA_4, 0, 594);
        auto chunk2 = reader.readCells(MeshInterface::TETRA_4, 594, ExpectedTetCount);
        auto recombined = chunk1;
        recombined.insert(recombined.end(), chunk2.begin(), chunk2.end());
        REQUIRE(tets == recombined);
        chunk1 = reader.readCells(MeshInterface::PYRA_5, 0, 75);
        chunk2 = reader.readCells(MeshInterface::PYRA_5, 75, ExpectedPyramidCount);
        recombined = chunk1;
        recombined.insert(recombined.end(), chunk2.begin(), chunk2.end());
        REQUIRE(pyramids == recombined);
    }

    int min_node_id = 9999999;
    int max_node_id = 0;
    for (int node_id : tris) {
        min_node_id = std::min(min_node_id, node_id);
        max_node_id = std::max(max_node_id, node_id);
    }
    for (int node_id : quads) {
        min_node_id = std::min(min_node_id, node_id);
        max_node_id = std::max(max_node_id, node_id);
    }
    for (int node_id : tets) {
        min_node_id = std::min(min_node_id, node_id);
        max_node_id = std::max(max_node_id, node_id);
    }
    for (int node_id : pyramids) {
        min_node_id = std::min(min_node_id, node_id);
        max_node_id = std::max(max_node_id, node_id);
    }
    for (int node_id : prisms) {
        min_node_id = std::min(min_node_id, node_id);
        max_node_id = std::max(max_node_id, node_id);
    }
    for (int node_id : hexs) {
        min_node_id = std::min(min_node_id, node_id);
        max_node_id = std::max(max_node_id, node_id);
    }

    REQUIRE(0 == min_node_id);
    REQUIRE((ExpectedNodeCount - 1) == max_node_id);
}

TEST_CASE("Read cell tags") {
    GmshReader reader(grid_file);
    auto tet_tags = reader.readCellTags(MeshInterface::TETRA_4);
    REQUIRE(ExpectedTetCount == tet_tags.size());
    auto tri_tags = reader.readCellTags(MeshInterface::TRI_3);
    REQUIRE(ExpectedTriCount == tri_tags.size());
    auto quad_tags = reader.readCellTags(MeshInterface::QUAD_4);
    REQUIRE(ExpectedQuadCount == quad_tags.size());
    auto pyramid_tags = reader.readCellTags(MeshInterface::PYRA_5);
    REQUIRE(ExpectedPyramidCount == pyramid_tags.size());
    auto prism_tags = reader.readCellTags(MeshInterface::PENTA_6);
    REQUIRE(ExpectedPrismCount == prism_tags.size());
    auto hex_tags = reader.readCellTags(MeshInterface::HEXA_8);
    REQUIRE(ExpectedHexCount == hex_tags.size());
}
