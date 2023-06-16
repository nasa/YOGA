#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/PointCloudInterpolator.h>
#include <t-infinity/MeshMover.h>
#include "t-infinity/Shortcuts.h"

double sin_function(double x, double y, double z) {
    double pi = 4.0 * atan(1.0);
    return sin(2 * pi * x) + cos(2 * pi * y) - sin(pi * z);
    //    return 3*x + 6*y - z;
}

std::vector<double> makeNodeTestField(const inf::MeshInterface& m,
                                      std::function<double(double, double, double)> test_function) {
    std::vector<double> field(m.nodeCount());

    for (int n = 0; n < m.nodeCount(); n++) {
        Parfait::Point<double> p = m.node(n);
        field[n] = test_function(p[0], p[1], p[2]);
    }

    return field;
}

double computeNodeError(const inf::MeshInterface& m,
                        const std::vector<double>& field,
                        std::function<double(double, double, double)> test_function) {
    double error = 0;
    for (int n = 0; n < m.nodeCount(); n++) {
        Parfait::Point<double> p = m.node(n);
        error += std::abs(test_function(p[0], p[1], p[2]) - field[n]);
    }
    return error / m.nodeCount();
}

TEST_CASE("Point cloud interpolator is 1st order") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto m1 = inf::CartMesh::create(3, 3, 3);
    auto m2 = inf::CartMesh::create(6, 6, 6);
    auto m3 = inf::CartMesh::create(12, 12, 12);
    auto f1 = makeNodeTestField(*m1, sin_function);
    auto f2 = makeNodeTestField(*m2, sin_function);
    auto f3 = makeNodeTestField(*m3, sin_function);

    std::vector<double> f2_approx, f3_approx;
    {
        inf::Interpolator interpolator(mp, m1, m2);
        f2_approx = interpolator.interpolateFromNodeData(f1);
    }
    {
        inf::Interpolator interpolator(mp, m2, m3);
        f3_approx = interpolator.interpolateFromNodeData(f2);
    }

    double error_2 = computeNodeError(*m2, f2_approx, sin_function);
    double error_3 = computeNodeError(*m3, f3_approx, sin_function);
    auto order = log(error_2 / error_3) / log(2);
    REQUIRE(order >= Approx(1.0));
}

TEST_CASE("Point cloud interpolator won't interpolate") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto smaller_mesh = inf::CartMesh::create(3, 3, 3);

    SECTION("Constant field") {
        auto f_smaller = std::vector<double>(smaller_mesh->nodeCount(), 1.24);
        auto bigger_mesh = inf::MeshMover::scale(smaller_mesh, 2.0);

        inf::Interpolator interpolator(mp, smaller_mesh, bigger_mesh);
        auto f_bigger = interpolator.interpolateFromNodeData(f_smaller);
        for (auto v : f_bigger) REQUIRE(1.24 == Approx(v));
    }
    SECTION("Linear field") {
        Parfait::Point<double> gradient(1.2, 3.5, 6.9);
        auto f_smaller = std::vector<double>(smaller_mesh->nodeCount());
        for (int node_id = 0; node_id < smaller_mesh->nodeCount(); ++node_id) {
            f_smaller[node_id] = Parfait::Point<double>::dot(gradient, smaller_mesh->node(node_id));
        }
        auto bigger_mesh = inf::MeshMover::scale(smaller_mesh, 2.0);

        inf::Interpolator interpolator(mp, smaller_mesh, bigger_mesh);
        auto f_bigger = interpolator.interpolateFromNodeData(f_smaller);
        for (int node_id = 0; node_id < bigger_mesh->nodeCount(); ++node_id) {
            auto expected = Parfait::Point<double>::dot(gradient, smaller_mesh->node(node_id));
            auto actual = f_smaller.at(node_id);
            REQUIRE(expected == Approx(actual));
        }
    }
    SECTION("Multi-element fields") {
        std::vector<double> donor_data(smaller_mesh->nodeCount() * 2);
        for (int n = 0; n < smaller_mesh->nodeCount(); n++) {
            donor_data[2 * n + 0] = 2;
            donor_data[2 * n + 1] = 9.81;
        }
        auto bigger_mesh = inf::MeshMover::scale(smaller_mesh, 2.0);

        inf::Interpolator interpolator(mp, smaller_mesh, bigger_mesh);
        auto f_bigger = interpolator.interpolate(std::make_shared<inf::VectorFieldAdapter>(
            "donor-data", inf::FieldAttributes::Node(), 2, donor_data));
        REQUIRE(f_bigger->blockSize() == 2);
        for (int n = 0; n < bigger_mesh->nodeCount(); n++) {
            auto v1 = f_bigger->getDouble(n, 0);
            auto v2 = f_bigger->getDouble(n, 1);
            REQUIRE(v1 == Approx(2.0));
            REQUIRE(v2 == Approx(9.81));
        }
    }
}

TEST_CASE("Can interpolate from a point cloud") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto donor_mesh = inf::CartMesh::create(mp, 25, 25, 25);
    auto receptor_mesh = inf::CartMesh::create(mp, 17, 17, 9);

    inf::FromPointCloudInterpolator interpolator(mp, donor_mesh, receptor_mesh);
    auto donor_field = makeNodeTestField(*donor_mesh, sin_function);
    auto interpolated_field = interpolator.interpolateFromNodeData(donor_field);

    REQUIRE(int(interpolator.stencils.size()) == receptor_mesh->nodeCount());

    for (int n = 0; n < receptor_mesh->nodeCount(); n++) {
        auto p = receptor_mesh->node(n);
        auto expected_value = sin_function(p[0], p[1], p[2]);
        auto interpolated_value = interpolated_field[n];
        REQUIRE(expected_value == Approx(interpolated_value).margin(0.2));
    }
}

TEST_CASE("Interpolate all scalars in a multi-element field") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto donor_mesh = inf::CartMesh::create(mp, 25, 25, 25);
    auto receptor_mesh = inf::CartMesh::create(mp, 17, 17, 9);

    inf::FromPointCloudInterpolator interpolator(mp, donor_mesh, receptor_mesh);
    std::vector<double> two_element_field(donor_mesh->nodeCount() * 2);
    for (int n = 0; n < donor_mesh->nodeCount(); n++) {
        auto p = donor_mesh->node(n);
        two_element_field[2 * n + 0] = sin_function(p[0], p[1], p[2]);
        two_element_field[2 * n + 1] = sin_function(p[0], p[1], p[2]);
    }
    auto donor_field = std::make_shared<inf::VectorFieldAdapter>(
        "two-element-field", inf::FieldAttributes::Node(), 2, two_element_field);
    auto interpolated_field = interpolator.interpolate(donor_field);
    REQUIRE(interpolated_field->association() == inf::FieldAttributes::Node());
    REQUIRE(interpolated_field->blockSize() == 2);

    for (int n = 0; n < receptor_mesh->nodeCount(); n++) {
        auto p = receptor_mesh->node(n);
        auto expected_value = sin_function(p[0], p[1], p[2]);
        auto v1 = interpolated_field->getDouble(n, 0);
        auto v2 = interpolated_field->getDouble(n, 1);
        REQUIRE(expected_value == Approx(v1).margin(0.2));
        REQUIRE(expected_value == Approx(v2).margin(0.2));
    }
}