#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/IsoSampling.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/Shortcuts.h>
#include <parfait/StringTools.h>

TEST_CASE("Sample a hex mesh") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create(mp, 2, 2, 2);
    auto plane = inf::isosampling::plane(*mesh, {0.5, 0.5, 0.5}, {1, 0, 0});
    auto filter = inf::isosampling::Isosurface(mp, mesh, plane);

    auto pika_density = std::make_shared<inf::VectorFieldAdapter>(
        "pika-density", inf::FieldAttributes::Node(), std::vector<double>(mesh->nodeCount(), 1.1));
    pika_density->setAdapterAttribute("units", "pika/m^2");

    auto filtered_pika_density = filter.apply(pika_density);
    REQUIRE(filtered_pika_density->attribute("units") == "pika/m^2");

    auto filtered_mesh = filter.getMesh();
    REQUIRE(filtered_mesh->nodeCount() == 9);
}

TEST_CASE("Sample a surface mesh") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::createSurface(mp, 2, 2, 2);
    auto plane = inf::isosampling::plane(*mesh, {0.2, 0.2, 0.2}, {0, 0, 1});
    auto filter = inf::isosampling::Isosurface(mp, mesh, plane);

    auto filtered_mesh = filter.getMesh();
    REQUIRE(filtered_mesh->cellCount() == 8);
    REQUIRE(filtered_mesh->cellCount() == filtered_mesh->cellCount(inf::MeshInterface::BAR_2));
    REQUIRE(filtered_mesh->nodeCount() == 8);

    auto pika_density = std::make_shared<inf::VectorFieldAdapter>(
        "pika-density", inf::FieldAttributes::Node(), std::vector<double>(mesh->nodeCount(), 1.1));
    pika_density->setAdapterAttribute("units", "pika/m^2");

    auto filtered_pika_density = filter.apply(pika_density);
    REQUIRE(filtered_pika_density->attribute("units") == "pika/m^2");
}

TEST_CASE("Sample a mesh with both volume and surface cells") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::createWithBarsAndNodes(mp, 2, 2, 2);
    auto plane = inf::isosampling::plane(*mesh, {0.2, 0.2, 0.2}, {0, 0, 1});
    auto filter = inf::isosampling::Isosurface(mp, mesh, plane);

    auto sliced_mesh = filter.getMesh();
    REQUIRE_NOTHROW(inf::shortcut::visualize(Parfait::StringTools::randomLetters(6), mp, sliced_mesh));
}

TEST_CASE("Sampling should create the same mesh in parallel and serial") {
    long parallel_node_count = 0;
    long parallel_cell_count = 0;
    long serial_node_count = 0;
    long serial_cell_count = 0;
    {
        auto mp = MessagePasser(MPI_COMM_WORLD);
        auto mesh = inf::CartMesh::createWithBarsAndNodes(mp, 2, 2, 2);
        auto plane = inf::isosampling::plane(*mesh, {0.2, 0.2, 0.2}, {0, 0, 1});
        auto filter = inf::isosampling::Isosurface(mp, mesh, plane);
        auto sliced_mesh = filter.getMesh();
        parallel_node_count = inf::globalNodeCount(mp, *sliced_mesh);
        parallel_cell_count = inf::globalCellCount(mp, *sliced_mesh);
    }
    {
        auto mp = MessagePasser(MPI_COMM_SELF);
        auto mesh = inf::CartMesh::createWithBarsAndNodes(mp, 2, 2, 2);
        auto plane = inf::isosampling::plane(*mesh, {0.2, 0.2, 0.2}, {0, 0, 1});
        auto filter = inf::isosampling::Isosurface(mp, mesh, plane);
        auto sliced_mesh = filter.getMesh();
        serial_node_count = inf::globalNodeCount(mp, *sliced_mesh);
        serial_cell_count = inf::globalCellCount(mp, *sliced_mesh);
    }

    REQUIRE(serial_node_count == parallel_node_count);
    REQUIRE(serial_cell_count == parallel_cell_count);
}