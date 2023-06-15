#include <RingAssertions.h>
#include <t-infinity/AddMissingFaces.h>
#include <t-infinity/Gradients.h>
#include "MockFields.h"

TEST_CASE("Node to Node gradients are exact for linear function", "[grad]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::shared_ptr<inf::MeshInterface> mesh = inf::CartMesh::create(mp, 5, 5, 5);

    auto linearField = [](double x, double y, double z) { return 4 * x + 3 * y - 1 * z; };
    auto linear_field_vector = inf::test::fillFieldAtNodes(*mesh, linearField);
    auto linear_field = inf::createNodeField(linear_field_vector);

    inf::NodeToNodeGradientCalculator calculator(mp, mesh);
    auto grad = calculator.calcGrad(*linear_field);
    for (int n = 0; n < mesh->nodeCount(); n++) {
        auto& node_grad = grad[n];
        REQUIRE(node_grad[0] == Approx(4.0));
        REQUIRE(node_grad[1] == Approx(3.0));
        REQUIRE(node_grad[2] == Approx(-1.0));
    }
}

TEST_CASE("Node to Node 2D gradients are exact for linear function", "[grad]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    std::shared_ptr<inf::MeshInterface> mesh = inf::CartMesh::create2D(mp, 5, 5);

    auto linearField = [](double x, double y, double z) { return 4 * x + 3 * y - 1 * z; };
    auto linear_field_vector = inf::test::fillFieldAtNodes(*mesh, linearField);
    auto linear_field = inf::createNodeField(linear_field_vector);

    inf::NodeToNodeGradientCalculator calculator(mp, mesh);
    auto grad = calculator.calcGrad(*linear_field);
    for (int n = 0; n < mesh->nodeCount(); n++) {
        auto& node_grad = grad[n];
        REQUIRE(node_grad[0] == Approx(4.0));
        REQUIRE(node_grad[1] == Approx(3.0));
        REQUIRE(node_grad[2] == Approx(0.0));
    }
}

TEST_CASE("Cell to node gradients are linearly accurate") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    std::shared_ptr<inf::MeshInterface> mesh = inf::CartMesh::create2D(mp, 5, 5);

    auto linearField = [](double x, double y, double z) { return 4 * x + 3 * y - 1 * z; };
    auto linear_field = inf::createCellField(inf::test::fillFieldAtCells(*mesh, linearField));

    inf::CellToNodeGradientCalculator calculator(mp, mesh);
    auto grad = calculator.calcGrad(*linear_field);
    for (auto node_grad : grad) {
        REQUIRE(node_grad[0] == Approx(4.0));
        REQUIRE(node_grad[1] == Approx(3.0));
        REQUIRE(node_grad[2] == Approx(0.0));
    }
}

TEST_CASE("Hessian calculation") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::shared_ptr<inf::MeshInterface> mesh = inf::CartMesh::create(5, 5, 5);

    auto linearField = [](double x, double y, double z) { return 4 * x + 3 * y - 1 * z; };
    auto field = inf::createCellField(inf::test::fillFieldAtCells(*mesh, linearField));
    inf::CellToNodeGradientCalculator calculator(mp, mesh);
    auto hess = calculator.calcHessian(*field);

    for (int node = 0; node < mesh->nodeCount(); node++) {
        REQUIRE(hess[node][0] == Approx(0.0).margin(1.0e-8));
        REQUIRE(hess[node][1] == Approx(0.0).margin(1.0e-8));
        REQUIRE(hess[node][2] == Approx(0.0).margin(1.0e-8));
        REQUIRE(hess[node][3] == Approx(0.0).margin(1.0e-8));
        REQUIRE(hess[node][4] == Approx(0.0).margin(1.0e-8));
        REQUIRE(hess[node][5] == Approx(0.0).margin(1.0e-8));
    }
}

TEST_CASE("Convert hessian to FieldInterface") {
    std::vector<std::array<double, 6>> hessian(2);
    hessian[0] = {1, 2, 3, 4, 5, 6};
    hessian[1] = {7, 7, 7, 7, 7, 7};
    auto hessian_field = inf::convertHessianToField(hessian);
    auto actual_size = hessian_field->size();
    REQUIRE(int(hessian.size()) == actual_size);
    std::array<double, 6> actual{};
    hessian_field->value(0, actual.data());
    REQUIRE(hessian.at(0) == actual);
    hessian_field->value(1, actual.data());
    REQUIRE(hessian.at(1) == actual);
}
