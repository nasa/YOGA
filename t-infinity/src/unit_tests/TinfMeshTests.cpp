#include <RingAssertions.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/MeshHelpers.h>
#include "MockMeshes.h"

using namespace inf;

TEST_CASE("Set cell in mixed element grid") {
    TinfMeshData mesh_data;

    mesh_data.cells[MeshInterface::TRI_3] = {0, 1, 2};
    mesh_data.cells[MeshInterface::QUAD_4] = {10, 11, 12, 13};
    mesh_data.cell_tags[MeshInterface::TRI_3] = {100};
    mesh_data.cell_tags[MeshInterface::QUAD_4] = {10};
    mesh_data.tag_names[100] = "dog";

    int mock_partition_id = 7;
    TinfMesh mesh(mesh_data, mock_partition_id);

    mesh.setCell(1, {110, 111, 112, 113});

    std::vector<int> quad(4);
    mesh.cell(1, quad.data());

    REQUIRE(quad[0] == 110);
    REQUIRE(quad[1] == 111);
    REQUIRE(quad[2] == 112);
    REQUIRE(quad[3] == 113);

    REQUIRE(mesh.cellTag(0) == 100);
    REQUIRE(mesh.cellTag(1) == 10);
    REQUIRE("Tag_10" == mesh.tagName(10));
    REQUIRE("dog" == mesh.tagName(100));
}

TEST_CASE("TinfMesh Copy Constructor") {
    auto mesh = mock::getSingleHexMeshWithFaces();
    mesh->setTagName(2, "pikachu");
    mesh->setTagName(4, "bulbasaur");
    TinfMesh copied_mesh(mesh);
    REQUIRE(mesh->nodeCount() == copied_mesh.nodeCount());
    REQUIRE(mesh->cellCount() == copied_mesh.cellCount());
    REQUIRE("Tag_1" == copied_mesh.tagName(1));
    REQUIRE("pikachu" == copied_mesh.tagName(2));
    REQUIRE("Tag_3" == copied_mesh.tagName(3));
    REQUIRE("bulbasaur" == copied_mesh.tagName(4));
    REQUIRE("Tag_5" == copied_mesh.tagName(5));
    REQUIRE("Tag_6" == copied_mesh.tagName(6));
}

TEST_CASE("Test base class global functions") {
    MessagePasser mp(MPI_COMM_WORLD);
    TinfMeshData mesh_data;

    mesh_data.cells[MeshInterface::TRI_3] = {0, 1, 2};
    long first_global_node_id = 3 * mp.Rank();
    mesh_data.node_owner = {mp.Rank(), mp.Rank(), mp.Rank()};
    mesh_data.global_node_id = {first_global_node_id, first_global_node_id + 1, first_global_node_id + 2};
    mesh_data.points.resize(3);
    mesh_data.global_cell_id[MeshInterface::TRI_3] = {mp.Rank()};
    mesh_data.cell_owner[MeshInterface::TRI_3] = {mp.Rank()};
    TinfMesh mesh(mesh_data, mp.Rank());

    long global_node_count = globalNodeCount(mp, mesh);
    long max_global_id = 3 * (mp.NumberOfProcesses() - 1) + 2;
    REQUIRE((max_global_id + 1) == global_node_count);
    long global_cell_count = globalCellCount(mp, mesh);
    long max_global_cell_id = mp.NumberOfProcesses();
    REQUIRE(max_global_cell_id == global_cell_count);
    REQUIRE(mp.NumberOfProcesses() == globalCellCount(mp, mesh, MeshInterface::TRI_3));
}

TEST_CASE("Node and cell count of empty mesh should be zero") {
    MessagePasser mp(MPI_COMM_WORLD);
    TinfMeshData mesh_data;
    TinfMesh mesh(mesh_data, mp.Rank());
    long global_node_count = globalNodeCount(mp, mesh);
    REQUIRE(global_node_count == 0);
    long global_cell_count = globalCellCount(mp, mesh);
    REQUIRE(global_cell_count == 0);
}

TEST_CASE("Check mesh is 2D") {
    MessagePasser mp(MPI_COMM_WORLD);

    auto single_triangle = TinfMesh(mock::oneTriangle(), mp.Rank());
    REQUIRE(is2D(mp, single_triangle));

    auto single_prism = TinfMesh(mock::onePrism(), mp.Rank());
    REQUIRE_FALSE(is2D(mp, single_prism));
}

TEST_CASE("Check if mesh is simplex") {
    MessagePasser mp(MPI_COMM_WORLD);

    auto single_tet = TinfMesh(mock::oneTet(), mp.Rank());
    REQUIRE(isSimplex(mp, single_tet));

    auto single_prism = TinfMesh(mock::onePrism(), mp.Rank());
    REQUIRE_FALSE(isSimplex(mp, single_prism));
}

TEST_CASE("Can determine mesh dimensionality") {
    MessagePasser mp(MPI_COMM_WORLD);

    auto single_tet = TinfMesh(mock::oneTet(), mp.Rank());
    REQUIRE(3 == maxCellDimensionality(mp, single_tet));

    auto single_triangle = TinfMesh(mock::oneTriangle(), mp.Rank());
    REQUIRE(2 == maxCellDimensionality(mp, single_triangle));
}
