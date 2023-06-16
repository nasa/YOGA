#include <RingAssertions.h>
#include "../Viz/SzPltWriter.h"

TEST_CASE("Can write hex27") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::createVolume(3, 3, 3);

    auto mesh = inf::elevateToP2(mp, *cart_mesh, inf::P1ToP2::getMapBiQuadratic());

    inf::SzWriter writer("cartmesh-biquadratic", *mesh);
    writer.write();
}

TEST_CASE("Can write tetra10") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::create(3, 3, 3);
    auto shard = std::make_shared<inf::TinfMesh>(inf::shortcut::shard(mp, *cart_mesh));

    auto mesh = inf::elevateToP2(mp, *shard, inf::P1ToP2::getMapBiQuadratic());
    std::vector<double> node_test_data(mesh->nodeCount());
    double pi = 4.0 * atan(1.0);
    for (int n = 0; n < mesh->nodeCount(); n++) {
        auto p = mesh->node(n);
        node_test_data[n] = cos(2 * pi * p[0]);
        node_test_data[n] += sin(2 * pi * p[1]);
        node_test_data[n] -= cos(2 * pi * p[2]);
    }

    std::vector<double> cell_test_data(mesh->cellCount());
    for (int c = 0; c < mesh->cellCount(); c++) {
        inf::Cell cell(mesh, c);
        auto p = cell.averageCenter();
        cell_test_data[c] = sin(pi * p[0]);
        cell_test_data[c] += cos(pi * p[1]);
        cell_test_data[c] -= sin(pi * p[2]);
    }

    auto node_field =
        std::make_shared<inf::VectorFieldAdapter>("test-node", inf::FieldAttributes::Node(), node_test_data);
    auto cell_field =
        std::make_shared<inf::VectorFieldAdapter>("test-cell", inf::FieldAttributes::Cell(), cell_test_data);

    inf::shortcut::visualize("tetra10", mp, mesh, {node_field, cell_field});

    inf::SzWriter writer("tetra10", *mesh);
    writer.addNodeField(node_field);
    writer.addCellField(cell_field);
    writer.write();
}