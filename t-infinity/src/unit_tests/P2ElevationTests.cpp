#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/ElevateMesh.h>
#include "t-infinity/MeshSanityChecker.h"
#include "t-infinity/Gradients.h"

TEST_CASE("Elevate Bar_2 to Bar_3") {
    using Type = inf::MeshInterface::CellType;
    std::vector<Parfait::Point<double>> bar_2 = {{0, 0, 0}, {1, 1, 1}};
    auto bar_3 = inf::P1ToP2::elevate(bar_2, Type::BAR_2, Type::BAR_3);
    REQUIRE(bar_3.size() == 3);
    REQUIRE(bar_3[0].approxEqual(bar_2[0]));
    REQUIRE(bar_3[1].approxEqual(0.5 * (bar_2[0] + bar_2[1])));
    REQUIRE(bar_3[2].approxEqual(bar_2[1]));
}

TEST_CASE("Elevate TRI_3 to TRI_6") {
    using Type = inf::MeshInterface::CellType;
    std::vector<Parfait::Point<double>> in_points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    auto out_points = inf::P1ToP2::elevate(in_points, Type::TRI_3, Type::TRI_6);
    REQUIRE(out_points.size() == 6);
    REQUIRE(out_points[0].approxEqual(in_points[0]));
    REQUIRE(out_points[1].approxEqual(in_points[1]));
    REQUIRE(out_points[2].approxEqual(in_points[2]));
    REQUIRE(out_points[3].approxEqual(0.5 * (in_points[0] + in_points[1])));
    REQUIRE(out_points[4].approxEqual(0.5 * (in_points[1] + in_points[2])));
    REQUIRE(out_points[5].approxEqual(0.5 * (in_points[2] + in_points[0])));
}

TEST_CASE("Elevate QUAD_4 to QUAD_8") {
    using Type = inf::MeshInterface::CellType;
    std::vector<Parfait::Point<double>> in_points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    auto out_points = inf::P1ToP2::elevate(in_points, Type::QUAD_4, Type::QUAD_8);
    REQUIRE(out_points.size() == 8);
    REQUIRE(out_points[0].approxEqual(in_points[0]));
    REQUIRE(out_points[1].approxEqual(in_points[1]));
    REQUIRE(out_points[2].approxEqual(in_points[2]));
    REQUIRE(out_points[3].approxEqual(in_points[3]));
    REQUIRE(out_points[4].approxEqual(0.5 * (in_points[0] + in_points[1])));
    REQUIRE(out_points[5].approxEqual(0.5 * (in_points[1] + in_points[2])));
    REQUIRE(out_points[6].approxEqual(0.5 * (in_points[2] + in_points[3])));
    REQUIRE(out_points[7].approxEqual(0.5 * (in_points[3] + in_points[0])));
}

TEST_CASE("Elevate QUAD_4 to QUAD_9") {
    using Type = inf::MeshInterface::CellType;
    std::vector<Parfait::Point<double>> in_points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    auto out_points = inf::P1ToP2::elevate(in_points, Type::QUAD_4, Type::QUAD_9);
    REQUIRE(out_points.size() == 9);
    REQUIRE(out_points[0].approxEqual(in_points[0]));
    REQUIRE(out_points[1].approxEqual(in_points[1]));
    REQUIRE(out_points[2].approxEqual(in_points[2]));
    REQUIRE(out_points[3].approxEqual(in_points[3]));
    REQUIRE(out_points[4].approxEqual(0.5 * (in_points[0] + in_points[1])));
    REQUIRE(out_points[5].approxEqual(0.5 * (in_points[1] + in_points[2])));
    REQUIRE(out_points[6].approxEqual(0.5 * (in_points[2] + in_points[3])));
    REQUIRE(out_points[7].approxEqual(0.5 * (in_points[3] + in_points[0])));
    REQUIRE(out_points[8].approxEqual(0.25 *
                                      (in_points[0] + in_points[1] + in_points[2] + in_points[3])));
}

TEST_CASE("Elevate TET_4 to TET_10") {
    using Type = inf::MeshInterface::CellType;
    std::vector<Parfait::Point<double>> in_points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    auto out_points = inf::P1ToP2::elevate(in_points, Type::TETRA_4, Type::TETRA_10);
    REQUIRE(out_points.size() == 10);
    REQUIRE(out_points[0].approxEqual(in_points[0]));
    REQUIRE(out_points[1].approxEqual(in_points[1]));
    REQUIRE(out_points[2].approxEqual(in_points[2]));
    REQUIRE(out_points[3].approxEqual(in_points[3]));
    REQUIRE(out_points[4].approxEqual(0.5 * (in_points[0] + in_points[1])));
    REQUIRE(out_points[5].approxEqual(0.5 * (in_points[1] + in_points[2])));
    REQUIRE(out_points[6].approxEqual(0.5 * (in_points[2] + in_points[0])));

    REQUIRE(out_points[7].approxEqual(0.5 * (in_points[0] + in_points[3])));
    REQUIRE(out_points[8].approxEqual(0.5 * (in_points[1] + in_points[3])));
    REQUIRE(out_points[9].approxEqual(0.5 * (in_points[2] + in_points[3])));
}

TEST_CASE("Elevate PYRA_5 to PYRA_13") {
    using Type = inf::MeshInterface::CellType;
    std::vector<Parfait::Point<double>> in_points = {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0.5, 0.5, 1.0}};
    auto out_points = inf::P1ToP2::elevate(in_points, Type::PYRA_5, Type::PYRA_13);
    REQUIRE(out_points.size() == 13);
    REQUIRE(out_points[0].approxEqual(in_points[0]));
    REQUIRE(out_points[1].approxEqual(in_points[1]));
    REQUIRE(out_points[2].approxEqual(in_points[2]));
    REQUIRE(out_points[3].approxEqual(in_points[3]));
    REQUIRE(out_points[4].approxEqual(in_points[4]));

    REQUIRE(out_points[5].approxEqual(0.5 * (in_points[0] + in_points[1])));
    REQUIRE(out_points[6].approxEqual(0.5 * (in_points[1] + in_points[2])));
    REQUIRE(out_points[7].approxEqual(0.5 * (in_points[2] + in_points[3])));
    REQUIRE(out_points[8].approxEqual(0.5 * (in_points[3] + in_points[0])));

    REQUIRE(out_points[9].approxEqual(0.5 * (in_points[0] + in_points[4])));
    REQUIRE(out_points[10].approxEqual(0.5 * (in_points[1] + in_points[4])));
    REQUIRE(out_points[11].approxEqual(0.5 * (in_points[2] + in_points[4])));
    REQUIRE(out_points[12].approxEqual(0.5 * (in_points[3] + in_points[4])));
}

TEST_CASE("Elevate PYRA_5 to PYRA_14") {
    using Type = inf::MeshInterface::CellType;
    std::vector<Parfait::Point<double>> in_points = {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0.5, 0.5, 1.0}};
    auto out_points = inf::P1ToP2::elevate(in_points, Type::PYRA_5, Type::PYRA_14);
    REQUIRE(out_points.size() == 14);
    REQUIRE(out_points[0].approxEqual(in_points[0]));
    REQUIRE(out_points[1].approxEqual(in_points[1]));
    REQUIRE(out_points[2].approxEqual(in_points[2]));
    REQUIRE(out_points[3].approxEqual(in_points[3]));
    REQUIRE(out_points[4].approxEqual(in_points[4]));

    REQUIRE(out_points[5].approxEqual(0.5 * (in_points[0] + in_points[1])));
    REQUIRE(out_points[6].approxEqual(0.5 * (in_points[1] + in_points[2])));
    REQUIRE(out_points[7].approxEqual(0.5 * (in_points[2] + in_points[3])));
    REQUIRE(out_points[8].approxEqual(0.5 * (in_points[3] + in_points[0])));

    REQUIRE(out_points[9].approxEqual(0.5 * (in_points[0] + in_points[4])));
    REQUIRE(out_points[10].approxEqual(0.5 * (in_points[1] + in_points[4])));
    REQUIRE(out_points[11].approxEqual(0.5 * (in_points[2] + in_points[4])));
    REQUIRE(out_points[12].approxEqual(0.5 * (in_points[3] + in_points[4])));
    REQUIRE(out_points[13].approxEqual(
        0.25 * (in_points[0] + in_points[1] + in_points[2] + in_points[3])));
}

TEST_CASE("Elevate PENTA_6 to PENTA_15") {
    using Type = inf::MeshInterface::CellType;
    std::vector<Parfait::Point<double>> in_points = {
        {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}};
    auto out_points = inf::P1ToP2::elevate(in_points, Type::PENTA_6, Type::PENTA_15);

    REQUIRE(out_points.size() == 15);
    REQUIRE(out_points[0].approxEqual(in_points[0]));
    REQUIRE(out_points[1].approxEqual(in_points[1]));
    REQUIRE(out_points[2].approxEqual(in_points[2]));
    REQUIRE(out_points[3].approxEqual(in_points[3]));
    REQUIRE(out_points[4].approxEqual(in_points[4]));
    REQUIRE(out_points[5].approxEqual(in_points[5]));

    REQUIRE(out_points[6].approxEqual(0.5 * (in_points[0] + in_points[1])));
    REQUIRE(out_points[7].approxEqual(0.5 * (in_points[1] + in_points[2])));
    REQUIRE(out_points[8].approxEqual(0.5 * (in_points[2] + in_points[0])));

    REQUIRE(out_points[9].approxEqual(0.5 * (in_points[0] + in_points[3])));
    REQUIRE(out_points[10].approxEqual(0.5 * (in_points[1] + in_points[4])));
    REQUIRE(out_points[11].approxEqual(0.5 * (in_points[2] + in_points[5])));

    REQUIRE(out_points[12].approxEqual(0.5 * (in_points[3] + in_points[4])));
    REQUIRE(out_points[13].approxEqual(0.5 * (in_points[4] + in_points[5])));
    REQUIRE(out_points[14].approxEqual(0.5 * (in_points[5] + in_points[3])));
}

TEST_CASE("Elevate PENTA_6 to PENTA_18") {
    using Type = inf::MeshInterface::CellType;
    std::vector<Parfait::Point<double>> in = {
        {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}};
    auto out = inf::P1ToP2::elevate(in, Type::PENTA_6, Type::PENTA_18);

    REQUIRE(out.size() == 18);
    REQUIRE(out[0].approxEqual(in[0]));
    REQUIRE(out[1].approxEqual(in[1]));
    REQUIRE(out[2].approxEqual(in[2]));
    REQUIRE(out[3].approxEqual(in[3]));
    REQUIRE(out[4].approxEqual(in[4]));
    REQUIRE(out[5].approxEqual(in[5]));

    REQUIRE(out[6].approxEqual(0.5 * (in[0] + in[1])));
    REQUIRE(out[7].approxEqual(0.5 * (in[1] + in[2])));
    REQUIRE(out[8].approxEqual(0.5 * (in[2] + in[0])));

    REQUIRE(out[9].approxEqual(0.5 * (in[0] + in[3])));
    REQUIRE(out[10].approxEqual(0.5 * (in[1] + in[4])));
    REQUIRE(out[11].approxEqual(0.5 * (in[2] + in[5])));

    REQUIRE(out[12].approxEqual(0.5 * (in[3] + in[4])));
    REQUIRE(out[13].approxEqual(0.5 * (in[4] + in[5])));
    REQUIRE(out[14].approxEqual(0.5 * (in[5] + in[3])));

    REQUIRE(out[15].approxEqual(0.25 * (in[0] + in[1] + in[3] + in[4])));
    REQUIRE(out[16].approxEqual(0.25 * (in[0] + in[2] + in[3] + in[5])));
    REQUIRE(out[17].approxEqual(0.25 * (in[2] + in[1] + in[5] + in[4])));
}

TEST_CASE("Elevate HEX_8 to HEX_20") {
    using Type = inf::MeshInterface::CellType;
    std::vector<Parfait::Point<double>> in = {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    auto out = inf::P1ToP2::elevate(in, Type::HEXA_8, Type::HEXA_20);

    REQUIRE(out.size() == 20);
    REQUIRE(out[0].approxEqual(in[0]));
    REQUIRE(out[1].approxEqual(in[1]));
    REQUIRE(out[2].approxEqual(in[2]));
    REQUIRE(out[3].approxEqual(in[3]));
    REQUIRE(out[4].approxEqual(in[4]));
    REQUIRE(out[5].approxEqual(in[5]));
    REQUIRE(out[6].approxEqual(in[5]));
    REQUIRE(out[7].approxEqual(in[5]));

    REQUIRE(out[8].approxEqual(0.5 * (in[0] + in[1])));
    REQUIRE(out[9].approxEqual(0.5 * (in[1] + in[2])));
    REQUIRE(out[10].approxEqual(0.5 * (in[2] + in[3])));
    REQUIRE(out[11].approxEqual(0.5 * (in[3] + in[0])));

    REQUIRE(out[12].approxEqual(0.5 * (in[0] + in[4])));
    REQUIRE(out[13].approxEqual(0.5 * (in[1] + in[5])));
    REQUIRE(out[14].approxEqual(0.5 * (in[2] + in[6])));
    REQUIRE(out[15].approxEqual(0.5 * (in[3] + in[7])));

    REQUIRE(out[16].approxEqual(0.5 * (in[4] + in[5])));
    REQUIRE(out[17].approxEqual(0.5 * (in[5] + in[6])));
    REQUIRE(out[18].approxEqual(0.5 * (in[6] + in[7])));
    REQUIRE(out[19].approxEqual(0.5 * (in[7] + in[4])));
}

TEST_CASE("Elevate HEX_8 to HEX_27") {
    using Type = inf::MeshInterface::CellType;
    std::vector<Parfait::Point<double>> in = {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    auto out = inf::P1ToP2::elevate(in, Type::HEXA_8, Type::HEXA_27);

    REQUIRE(out.size() == 27);
    REQUIRE(out[0].approxEqual(in[0]));
    REQUIRE(out[1].approxEqual(in[1]));
    REQUIRE(out[2].approxEqual(in[2]));
    REQUIRE(out[3].approxEqual(in[3]));
    REQUIRE(out[4].approxEqual(in[4]));
    REQUIRE(out[5].approxEqual(in[5]));
    REQUIRE(out[6].approxEqual(in[5]));
    REQUIRE(out[7].approxEqual(in[5]));

    REQUIRE(out[8].approxEqual(0.5 * (in[0] + in[1])));
    REQUIRE(out[9].approxEqual(0.5 * (in[1] + in[2])));
    REQUIRE(out[10].approxEqual(0.5 * (in[2] + in[3])));
    REQUIRE(out[11].approxEqual(0.5 * (in[3] + in[0])));

    REQUIRE(out[12].approxEqual(0.5 * (in[0] + in[4])));
    REQUIRE(out[13].approxEqual(0.5 * (in[1] + in[5])));
    REQUIRE(out[14].approxEqual(0.5 * (in[2] + in[6])));
    REQUIRE(out[15].approxEqual(0.5 * (in[3] + in[7])));

    REQUIRE(out[16].approxEqual(0.5 * (in[4] + in[5])));
    REQUIRE(out[17].approxEqual(0.5 * (in[5] + in[6])));
    REQUIRE(out[18].approxEqual(0.5 * (in[6] + in[7])));
    REQUIRE(out[19].approxEqual(0.5 * (in[7] + in[4])));

    REQUIRE(out[20].approxEqual(0.25 * (in[0] + in[1] + in[2] + in[3])));

    REQUIRE(out[21].approxEqual(0.25 * (in[0] + in[4] + in[1] + in[5])));
    REQUIRE(out[22].approxEqual(0.25 * (in[1] + in[5] + in[2] + in[6])));
    REQUIRE(out[23].approxEqual(0.25 * (in[2] + in[6] + in[3] + in[7])));
    REQUIRE(out[24].approxEqual(0.25 * (in[3] + in[7] + in[0] + in[4])));

    REQUIRE(out[25].approxEqual(0.25 * (in[4] + in[5] + in[6] + in[7])));

    REQUIRE(out[26].approxEqual(0.125 *
                                (in[0] + in[1] + in[2] + in[3] + in[4] + in[5] + in[6] + in[7])));
}

TEST_CASE("Can extract types from a mesh") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create(mp, 5, 5, 5);
    auto types = inf::extractAllCellTypes(mp, *mesh);
    REQUIRE(types.size() == 2);
    REQUIRE(types.count(inf::MeshInterface::QUAD_4) == 1);
}

TEST_CASE("Map P1 to P2 Quadratic") {
    auto p1_to_p2_map = inf::P1ToP2::getMapQuadratic();
    using Type = inf::MeshInterface::CellType;
    REQUIRE(p1_to_p2_map.at(Type::NODE) == Type::NODE);
    REQUIRE(p1_to_p2_map.at(Type::TRI_3) == Type::TRI_6);
    REQUIRE(p1_to_p2_map.at(Type::QUAD_4) == Type::QUAD_8);
    REQUIRE(p1_to_p2_map.at(Type::TETRA_4) == Type::TETRA_10);
    REQUIRE(p1_to_p2_map.at(Type::PYRA_5) == Type::PYRA_13);
    REQUIRE(p1_to_p2_map.at(Type::PENTA_6) == Type::PENTA_15);
    REQUIRE(p1_to_p2_map.at(Type::HEXA_8) == Type::HEXA_20);
}

TEST_CASE("Map P1 to P2 BiQuadratic") {
    auto p1_to_p2_map = inf::P1ToP2::getMapBiQuadratic();
    using Type = inf::MeshInterface::CellType;
    REQUIRE(p1_to_p2_map.at(Type::NODE) == Type::NODE);
    REQUIRE(p1_to_p2_map.at(Type::TRI_3) == Type::TRI_6);
    REQUIRE(p1_to_p2_map.at(Type::QUAD_4) == Type::QUAD_9);
    REQUIRE(p1_to_p2_map.at(Type::TETRA_4) == Type::TETRA_10);
    REQUIRE(p1_to_p2_map.at(Type::PYRA_5) == Type::PYRA_14);
    REQUIRE(p1_to_p2_map.at(Type::PENTA_6) == Type::PENTA_18);
    REQUIRE(p1_to_p2_map.at(Type::HEXA_8) == Type::HEXA_27);
}
TEST_CASE("Can elevate a single Hex") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::createVolume(mp, 1, 1, 1);
    REQUIRE(mesh->cellCount(inf::MeshInterface::HEXA_8) == 1);

    auto p2_mesh = inf::elevateToP2(mp, *mesh, inf::P1ToP2::getMapQuadratic());

    REQUIRE(mesh->tagName(1) == p2_mesh->tagName(1));

    REQUIRE(p2_mesh->cellCount() == mesh->cellCount());
    REQUIRE(p2_mesh->cellCount(inf::MeshInterface::HEXA_20) ==
            mesh->cellCount(inf::MeshInterface::HEXA_8));

    REQUIRE(p2_mesh->nodeCount() == 20);
}
TEST_CASE("Can visualize a P2 hex mesh", "[P2-debug]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create(mp, 2, 2, 2);

    auto p2_mesh = inf::elevateToP2(mp, *mesh);

    for (auto& p : p2_mesh->mesh.points) {
        double x = p[0];
        double y = p[1];
        double z = p[2];
        p[0] = x * sqrt(1.0 - y * y / 2.0 - z * z / 2.0 + y * y * z * z / 3.0);
        p[1] = y * sqrt(1.0 - x * x / 2.0 - z * z / 2.0 + x * x * z * z / 3.0);
        p[2] = z * sqrt(1.0 - x * x / 2.0 - y * y / 2.0 + x * x * y * y / 3.0);
    }

    double pi = 4.0 * atan(1.0);
    auto test_function = [=](double x, double y, double z) {
        return sin(pi * x) + cos(pi * y) - sin(pi * z);
    };

    std::vector<double> test_data(p2_mesh->nodeCount());
    for (int n = 0; n < p2_mesh->nodeCount(); n++) {
        auto p = p2_mesh->node(n);
        test_data[n] = test_function(p[0], p[1], p[2]);
    }

    inf::shortcut::visualize_at_nodes("hex-p2", mp, p2_mesh, {test_data});
}

TEST_CASE("Can interpolate a cell field to a P2 node field") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto p1_mesh = std::make_shared<inf::TinfMesh>(
        inf::shortcut::shard(mp, *inf::CartMesh::create(mp, 5, 5, 5)));

    auto p2_mesh = inf::elevateToP2(mp, *p1_mesh, inf::P1ToP2::getMapQuadratic());

    double pi = 4.0 * atan(1.0);
    auto test_function = [=](double x, double y, double z) {
        return cos(2.0 * pi * x) + sin(2.0 * pi * y) - cos(2.0 * pi * z);
    };

    std::vector<double> test_data(p1_mesh->cellCount());
    for (int c = 0; c < p1_mesh->cellCount(); c++) {
        inf::Cell cell(p1_mesh, c);
        auto p = cell.averageCenter();
        test_data[c] = test_function(p[0], p[1], p[2]);
    }

    auto p1_cell_field = std::make_shared<inf::VectorFieldAdapter>(
        "test-p1-cell", inf::FieldAttributes::Cell(), test_data);

    double distance_weight = 0.0;
    inf::CellToNodeGradientCalculator nodal_gradient_calculator(mp, p1_mesh, distance_weight);

    auto n2c_p2 = inf::NodeToCell::build(*p2_mesh);

    auto p2_node_field = inf::interpolateCellFieldToNodes(
        *p1_mesh, *p2_mesh, nodal_gradient_calculator, n2c_p2, *p1_cell_field);

    std::vector<double> sampled_data(p2_mesh->nodeCount());
    for (int n = 0; n < p2_mesh->nodeCount(); n++) {
        auto p = p2_mesh->node(n);
        sampled_data[n] = test_function(p[0], p[1], p[2]);
    }

    auto p2_node_field_sampled = std::make_shared<inf::VectorFieldAdapter>(
        "sampled-field", inf::FieldAttributes::Node(), sampled_data);

    auto p1_node_averaged_field =
        inf::FieldTools::cellToNode_simpleAverage(*p1_mesh, *p1_cell_field);
    p1_node_averaged_field->setAdapterAttribute(inf::FieldAttributes::name(),
                                                "p1_node_field_simple_avg");

    auto diff = inf::FieldTools::diff(*p2_node_field, *p2_node_field_sampled);

    //    inf::shortcut::visualize(
    //        "hex-p1-cell-data.szplt", mp, p1_mesh, {p1_cell_field, p1_node_averaged_field});
    //    inf::shortcut::visualize("hex-p2-nodal-reconstruction.szplt",
    //                             mp,
    //                             p2_mesh,
    //                             {p2_node_field, p2_node_field_sampled, diff});
}
