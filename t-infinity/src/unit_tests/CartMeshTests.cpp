#include <RingAssertions.h>
#include <t-infinity/Extract.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Shortcuts.h>
#include "t-infinity/MeshHelpers.h"
#include <t-infinity/MeshPrinter.h>

TEST_CASE("Create a tinf mesh from a cart block") {
    auto mesh = inf::CartMesh::createVolume(10, 5, 6);
    REQUIRE(mesh->cell_count == 10 * 5 * 6);
}

TEST_CASE("Create tinf mesh from cart block including faces") {
    auto mesh = inf::CartMesh::create(10, 5, 6);
    long volume_count = 10 * 5 * 6;
    long xy_count = 2 * 10 * 5;
    long xz_count = 2 * 10 * 6;
    long yz_count = 2 * 5 * 6;
    long surface_count = xy_count + xz_count + yz_count;
    std::set<int> surface_tags = {1, 2, 3, 4, 5, 6};
    REQUIRE(mesh->nodeCount() == 11 * 6 * 7);
    REQUIRE(mesh->cell_count == volume_count + surface_count);
    REQUIRE(mesh->cellCount() == volume_count + surface_count);
    REQUIRE(mesh->cellCount(inf::MeshInterface::QUAD_4) == surface_count);
    REQUIRE(inf::extractTagsWithDimensionality(*mesh, 2) == surface_tags);
}

TEST_CASE("Create tinf mesh from cart block and partition") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 4, 4, 4);
    long total_cells = 64;
    long total_nodes = 125;
    REQUIRE(total_cells == inf::globalCellCount(mp, *mesh, inf::MeshInterface::HEXA_8));
    REQUIRE(total_nodes == inf::globalNodeCount(mp, *mesh));
    REQUIRE(mp.Rank() == mesh->partitionId());
}

TEST_CASE("Create a surface only cartmesh"){
    auto mesh = inf::CartMesh::createSurface(4, 4, 4);
    REQUIRE(mesh->cellCount() == 6*16);
}

TEST_CASE("Sphere cartmesh") {
    auto mesh = inf::CartMesh::createSphere(20, 10, 10);
}

TEST_CASE("Create a cartmesh with bar edges") {
    auto mesh = inf::CartMesh::createWithBarsAndNodes(1, 1, 1);
    REQUIRE(mesh->cellCount(inf::MeshInterface::NODE) == 8);
    REQUIRE(mesh->cellCount(inf::MeshInterface::BAR_2) == 12);
    REQUIRE(mesh->cellCount(inf::MeshInterface::QUAD_4) == 6);
    REQUIRE(mesh->cellCount(inf::MeshInterface::HEXA_8) == 1);
    REQUIRE(mesh->cellCount() == 8 + 12 + 6 + 1);
}
TEST_CASE("Create a cartmesh with bar edges in parallel") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::createWithBarsAndNodes(mp, 1, 1, 1);
    REQUIRE(mesh->cellCount(inf::MeshInterface::NODE) == 8);
    REQUIRE(mesh->cellCount(inf::MeshInterface::BAR_2) == 12);
    REQUIRE(mesh->cellCount(inf::MeshInterface::QUAD_4) == 6);
    REQUIRE(mesh->cellCount(inf::MeshInterface::HEXA_8) == 1);
    REQUIRE(mesh->cellCount() == 8 + 12 + 6 + 1);
}

TEST_CASE("Create 2D cartmesh") {
    auto mesh = inf::CartMesh::create2D(2,2);
    REQUIRE(mesh != nullptr);
    REQUIRE(mesh->cellCount(inf::MeshInterface::QUAD_4) == 4);
    REQUIRE(mesh->nodeCount() == 9);
}

TEST_CASE("Create 2D cartmesh with bars and nodes") {

    SECTION("single quad case"){
        auto mesh = inf::CartMesh::create2DWithBarsAndNodes(1, 1);
        REQUIRE(mesh != nullptr);
        bool found = false;
        for(int c = 0; c < mesh->cellCount(); c++){
            if(mesh->cellType(c) == inf::MeshInterface::BAR_2){
                auto cell = mesh->cell(c);
                if(cell[0] == 1 and cell[1] == 0) found = true;
                if(cell[0] == 0 and cell[1] == 1) found = true;
            }
        }
        REQUIRE(found);
    }
    SECTION("2 quad case"){
        auto mesh = inf::CartMesh::create2DWithBarsAndNodes(2, 2);
        REQUIRE(mesh != nullptr);
        REQUIRE(mesh->cellCount(inf::MeshInterface::QUAD_4) == 4);
        REQUIRE(mesh->cellCount(inf::MeshInterface::BAR_2) == 8);
        REQUIRE(mesh->cellCount(inf::MeshInterface::NODE) == 4);
        REQUIRE(mesh->nodeCount() == 9);
    }
}

