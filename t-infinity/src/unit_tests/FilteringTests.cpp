#include <MessagePasser/MessagePasser.h>
#include <t-infinity/CellIdFilter.h>
#include <t-infinity/CellSelectedMesh.h>
#include <t-infinity/ExtentIntersections.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/Extract.h>
#include <t-infinity/FilterFactory.h>
#include <t-infinity/CartMesh.h>
#include <RingAssertions.h>
#include "MockMeshes.h"

TEST_CASE("Sphere Extent Intersection") {
    Parfait::Point<double> center{0, 0, 0};
    Parfait::Extent<double> e({0, 0, 0}, {1, 1, 1});
    REQUIRE(inf::sphereExtentIntersection(1.0, center, e));
    REQUIRE(inf::sphereExtentIntersection(1.0, center, {{.9, 0, 0}, {2, 1, 1}}));
    REQUIRE_FALSE(inf::sphereExtentIntersection(0.8, center, {{.9, 0, 0}, {2, 1, 1}}));

    double root_2 = std::sqrt(2.0) / 2.0;
    Parfait::Point<double> in{root_2, root_2, 0};
    REQUIRE(inf::sphereExtentIntersection(1.0, center, {in, in}));

    double nudge = 0.005;
    Parfait::Point<double> out{root_2 + nudge, root_2 + nudge, 0};
    REQUIRE_FALSE(inf::sphereExtentIntersection(1.0, center, {out, out}));
}

TEST_CASE("Cell Id Filtering") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto input_mesh = std::make_shared<inf::mock::TwoTetMesh>();

    std::vector<int> selected_cell_ids = {1};
    auto filter = inf::CellIdFilter(mp.getCommunicator(), input_mesh, selected_cell_ids);
    auto filtered_mesh = filter.getMesh();

    REQUIRE(1 == filtered_mesh->cellCount());
    REQUIRE(4 == filtered_mesh->nodeCount());
}

TEST_CASE("Cell Id field filtering") {
    auto input_mesh = std::make_shared<inf::mock::TwoTetMesh>();
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<double> scalar_field = {0, 1, 2, 3, 4, 5, 6, 7};

    auto input_field =
        std::make_shared<inf::VectorFieldAdapter>("scalar", inf::FieldAttributes::Node(), 1, scalar_field);
    REQUIRE(input_field->size() == input_mesh->nodeCount());

    std::vector<int> selected_cell_ids = {1};
    auto filter = inf::CellIdFilter(mp.getCommunicator(), input_mesh, selected_cell_ids);
    auto filtered_mesh = filter.getMesh();
    auto filtered_field = filter.apply(input_field);

    REQUIRE(4 == filtered_field->size());
    double v;
    filtered_field->value(0, &v);
    REQUIRE(4 == Approx(v));
}

TEST_CASE("Cell Id field filtering on a cell field") {
    auto input_mesh = std::make_shared<inf::mock::TwoTetMesh>();
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<double> scalar_field = {0, 1};

    auto input_field =
        std::make_shared<inf::VectorFieldAdapter>("scalar", inf::FieldAttributes::Cell(), 1, scalar_field);
    REQUIRE(input_field->size() == input_mesh->cellCount());

    std::vector<int> selected_cell_ids = {1};
    auto filter = inf::CellIdFilter(mp.getCommunicator(), input_mesh, selected_cell_ids);
    auto filtered_mesh = filter.getMesh();
    auto filtered_field = filter.apply(input_field);

    REQUIRE(1 == filtered_field->size());
    double v;
    filtered_field->value(0, &v);
    REQUIRE(1 == Approx(v));
}

TEST_CASE("Sphere Filter ") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto input_mesh = std::make_shared<inf::mock::TwoTetMesh>();
    auto sphere_filter = inf::FilterFactory::clipSphereFilter(mp.getCommunicator(), input_mesh, 0.1, {0, 0, 0});

    auto mesh = sphere_filter->getMesh();
}

TEST_CASE("Extent Filter ") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto input_mesh = std::make_shared<inf::mock::TwoTetMesh>();
    auto extent_filter =
        inf::FilterFactory::clipExtentFilter(mp.getCommunicator(), input_mesh, {{0, 0, 0}, {1, 1, 1}});

    auto mesh = extent_filter->getMesh();
}

TEST_CASE("Plane filter") {
    MessagePasser mp(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create(mp, 2, 2, 2);
    Parfait::Point<double> center = {0.25,0,0};
    Parfait::Point<double> normal = {1,0,0};
    auto filter =
        inf::FilterFactory::clipPlaneFilter(mp.getCommunicator(), mesh, center, normal);
    auto filtered_mesh = filter->getMesh();
    REQUIRE(filtered_mesh->cellCount(inf::MeshInterface::HEXA_8) ==  4);
}

TEST_CASE("Tag filter") {
    MessagePasser mp(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create(mp, 2, 2, 2);
    SECTION("Filter tags by vector") {
        std::vector<int> tags = {1,5};
        auto filter = inf::FilterFactory::tagFilter(mp.getCommunicator(), mesh, tags);
        auto filtered_mesh = filter->getMesh();
        REQUIRE(filtered_mesh->cellCount(inf::MeshInterface::HEXA_8) == 0);
        REQUIRE(filtered_mesh->cellCount(inf::MeshInterface::QUAD_4) == 8);
    }
    SECTION("Filter tags by set") {
        std::set<int> tags = {1,5};
        auto filter = inf::FilterFactory::tagFilter(mp.getCommunicator(), mesh, tags);
        auto filtered_mesh = filter->getMesh();
        REQUIRE(filtered_mesh->cellCount(inf::MeshInterface::HEXA_8) == 0);
        REQUIRE(filtered_mesh->cellCount(inf::MeshInterface::QUAD_4) == 8);
    }
}

TEST_CASE("Tag filter will give volume cells"){
    MessagePasser mp(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create(mp, 2, 2, 2);
    std::vector<int> tags = {0,5};
    auto filter = inf::FilterFactory::tagFilter(mp.getCommunicator(), mesh, tags);
    auto filtered_mesh = filter->getMesh();
    REQUIRE(filtered_mesh->cellCount(inf::MeshInterface::HEXA_8) == 8);
    REQUIRE(filtered_mesh->cellCount(inf::MeshInterface::QUAD_4) == 4);
}

TEST_CASE("SurfaceTag filter will not give volume cells"){
    MessagePasser mp(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create(mp, 2, 2, 2);
    std::vector<int> tags = {0,5};
    auto filter = inf::FilterFactory::surfaceTagFilter(mp.getCommunicator(), mesh, tags);
    auto filtered_mesh = filter->getMesh();
    REQUIRE(filtered_mesh->cellCount(inf::MeshInterface::HEXA_8) == 0);
    REQUIRE(filtered_mesh->cellCount(inf::MeshInterface::QUAD_4) == 4);
}


TEST_CASE("Filter of filter") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto full_mesh = inf::CartMesh::create(mp, 10, 10, 10, {{0, 0, 0}, {1, 1, 1}});
    auto extent_filter =
        inf::FilterFactory::clipExtentFilter(mp.getCommunicator(), full_mesh, {{0, 0, 0}, {.5, .5, .5}});
    auto mesh = extent_filter->getMesh();
}

TEST_CASE("Filtered mesh forwards boundary names") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 4, 4, 4);
    auto tags = inf::extractAllTagsWithDimensionality(mp, *mesh, 2);
    for (auto t : tags) {
        mesh->setTagName(t, "pikachu-" + std::to_string(t));
    }
    auto filter = inf::FilterFactory::surfaceFilter(mp.getCommunicator(), mesh);
    auto filtered_mesh = filter->getMesh();
    for (auto t : tags) {
        REQUIRE(filtered_mesh->tagName(t) == "pikachu-" + std::to_string(t));
    }
}

TEST_CASE("Filtering field forwards attributes") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 4, 4, 4);
    auto tags = inf::extractAllTagsWithDimensionality(mp, *mesh, 2);
    auto filter = inf::FilterFactory::surfaceFilter(mp.getCommunicator(), mesh);
    auto filtered_mesh = filter->getMesh();
    auto pika_density = std::make_shared<inf::VectorFieldAdapter>(
        "pika-density", inf::FieldAttributes::Node(), std::vector<double>(mesh->nodeCount(), 1.1));
    pika_density->setAdapterAttribute("units", "pika/m^2");

    auto filtered_pika_density = filter->apply(pika_density);
    REQUIRE(filtered_pika_density->attribute("units") == "pika/m^2");
}