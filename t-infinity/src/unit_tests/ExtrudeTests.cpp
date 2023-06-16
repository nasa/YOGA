#include <RingAssertions.h>
#include "MockMeshes.h"
#include <parfait/ToString.h>
#include <t-infinity/MeshExtruder.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Extract.h>
#include <t-infinity/Shortcuts.h>

TEST_CASE("Extrude a triangle all the ways we can think of") {
    std::vector<int> cell_nodes = {0, 1, 2};
    SECTION("All nodes are new") {
        std::vector<int> old_to_new = {3, 4, 5};
        auto cell = inf::extrude::extrudeTriangle(cell_nodes, old_to_new);
        REQUIRE(cell.type == inf::MeshInterface::PENTA_6);
        REQUIRE(cell.cell_nodes == std::vector<int>{0, 1, 2, 3, 4, 5});
    }
    SECTION("First node is collapsed") {
        std::vector<int> old_to_new = {0, 4, 5};
        auto cell = inf::extrude::extrudeTriangle(cell_nodes, old_to_new);
        REQUIRE(cell.type == inf::MeshInterface::PYRA_5);
        REQUIRE(cell.cell_nodes == std::vector<int>{1, 4, 5, 2, 0});
    }
    SECTION("Second node is collapsed") {
        std::vector<int> old_to_new = {3, 1, 5};
        auto cell = inf::extrude::extrudeTriangle(cell_nodes, old_to_new);
        REQUIRE(cell.type == inf::MeshInterface::PYRA_5);
        REQUIRE(cell.cell_nodes == std::vector<int>{2, 5, 3, 0, 1});
    }
    SECTION("Third node is collapsed") {
        std::vector<int> old_to_new = {3, 4, 2};
        auto cell = inf::extrude::extrudeTriangle(cell_nodes, old_to_new);
        REQUIRE(cell.type == inf::MeshInterface::PYRA_5);
        REQUIRE(cell.cell_nodes == std::vector<int>{0, 3, 4, 1, 2});
    }

    SECTION("0 1 edge is collapsed") {
        std::vector<int> old_to_new = {0, 1, 5};
        auto cell = inf::extrude::extrudeTriangle(cell_nodes, old_to_new);
        REQUIRE(cell.type == inf::MeshInterface::TETRA_4);
        REQUIRE(cell.cell_nodes == std::vector<int>{0, 1, 2, 5});
    }

    SECTION("1 2 edge is collapsed") {
        std::vector<int> old_to_new = {3, 1, 2};
        auto cell = inf::extrude::extrudeTriangle(cell_nodes, old_to_new);
        REQUIRE(cell.type == inf::MeshInterface::TETRA_4);
        REQUIRE(cell.cell_nodes == std::vector<int>{0, 1, 2, 3});
    }

    SECTION("2 0 edge is collapsed") {
        std::vector<int> old_to_new = {0, 4, 2};
        auto cell = inf::extrude::extrudeTriangle(cell_nodes, old_to_new);
        REQUIRE(cell.type == inf::MeshInterface::TETRA_4);
        REQUIRE(cell.cell_nodes == std::vector<int>{1, 2, 0, 4});
    }
}

TEST_CASE("Extrude a bar") {
    std::vector<int> cell_nodes = {0, 1};
    SECTION("All nodes are new") {
        std::vector<int> old_to_new = {3, 4};
        auto cell = inf::extrude::extrudeBar(cell_nodes, old_to_new);
        REQUIRE(cell.type == inf::MeshInterface::QUAD_4);
        REQUIRE(cell.cell_nodes == std::vector<int>{0, 1, 4, 3});
    }
    SECTION("No nodes are new") {
        // This occurs at axisymmetric boundaries
        std::vector<int> old_to_new = {0, 1};
        auto cell = inf::extrude::extrudeBar(cell_nodes, old_to_new);
        REQUIRE(cell.is_null);
    }
    SECTION("one side collapsed") {
        std::vector<int> old_to_new = {0, 4};
        auto cell = inf::extrude::extrudeBar(cell_nodes, old_to_new);
        REQUIRE(cell.type == inf::MeshInterface::TRI_3);
        REQUIRE(cell.cell_nodes == std::vector<int>{0, 1, 4});
    }
}

TEST_CASE("Can extrude a 2D mesh") {
    using Type = inf::extrude::Type;
    MessagePasser mp(MPI_COMM_SELF);
    int partition_id = 0;
    auto mesh = inf::TinfMesh(inf::mock::oneTriangle(), partition_id);
    REQUIRE(mesh.nodeCount() == 3);
    REQUIRE(mesh.cellCount() == 1);

    auto extruded = inf::extrude::extrudeAxisymmetric(mp, mesh, Type::AXISYMMETRIC_X, 15);

    REQUIRE(extruded->nodeCount() == 4);
    REQUIRE(extruded->cellCount() == 5);
}

TEST_CASE("Can extrude a 2D mesh and maintain boundary tags") {
    using Type = inf::extrude::Type;
    MessagePasser mp(MPI_COMM_SELF);
    auto cart = inf::CartMesh::create2DWithBars(2, 2);
    auto ref_mesh = inf::shortcut::shard(mp, *cart);
    auto mesh = std::make_shared<inf::TinfMesh>(ref_mesh);
    auto& bar_tags = mesh->mesh.cell_tags[inf::MeshInterface::BAR_2];
    int num_bars = bar_tags.size();
    for (int b = 0; b < num_bars; b++) {
        bar_tags[b] = b + 1;
        mesh->setTagName(b + 1, "bar_" + std::to_string(b + 1));
    }

    auto extruded = inf::extrude::extrudeAxisymmetric(mp, *mesh, Type::AXISYMMETRIC_X, 15);

    auto orig_tags = inf::extract1DTagsToNames(mp, *mesh);
    auto tags = inf::extract2DTagsToNames(mp, *extruded);
    int num_axisymmetric_bars = 2;
    int num_symmetry_planes = 2;
    int num_tags = tags.size();
    REQUIRE(num_tags == num_bars - num_axisymmetric_bars + num_symmetry_planes);
    // Require the tags are an ordinal set
    for (int t = 1; t < num_tags; t++) {
        REQUIRE(tags.count(t));
    }
    REQUIRE(tags.at(1) == "bar_1");
    REQUIRE(tags.at(2) == "bar_4");
    REQUIRE(tags.at(3) == "bar_5");
    REQUIRE(tags.at(4) == "bar_6");
    REQUIRE(tags.at(5) == "bar_7");
    REQUIRE(tags.at(6) == "bar_8");
    REQUIRE(tags.at(7) == "symm-1");
    REQUIRE(tags.at(8) == "symm-2");
}
