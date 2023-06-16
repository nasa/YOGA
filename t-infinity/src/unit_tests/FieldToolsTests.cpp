#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/FaceNeighbors.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/MDVector.h>
#include <t-infinity/MeshHelpers.h>

using namespace inf;

TEST_CASE("Will copy data from volume cell to surface cell") {
    auto mesh = CartMesh::create(2, 2, 2);
    std::vector<int> some_vector(mesh->cellCount());

    for (int c = 0; c < mesh->cellCount(); c++) {
        some_vector[c] = c;
    }

    auto face_neighbors = FaceNeighbors::buildOnlyForCellsWithDimensionality(*mesh, 2);

    FieldTools::setSurfaceCellToVolumeNeighborValue(*mesh, face_neighbors, 2, some_vector);
    for (int c = 0; c < mesh->cellCount(); c++) {
        if (mesh->is2DCell(c)) {
            REQUIRE(some_vector[c] == face_neighbors[c][0]);
        }
    }
}

TEST_CASE("Calculate u+ for Spalding's law of the wall") {
    double yplus = 0;
    for (int i = 0; i < 1000; ++i) {
        auto uplus = FieldTools::calcSpaldingLawOfTheWallUPlus(yplus);
        double error = FieldTools::spaldingYPlus(uplus) - yplus;
        REQUIRE(std::abs(error) < 1e-6);
        yplus += 0.005;
    }
}
TEST_CASE("Calculate u+ for Spalding's law of the wall from distance field") {
    std::vector<double> wall_distance = {0.0, 1.5, 2000.0};
    double spalding_yplus = 0.01;
    auto uplus = FieldTools::calcSpaldingLawOfTheWallUPlus(spalding_yplus, wall_distance);
    REQUIRE(3 == uplus.size());
    for (int i = 0; i < 3; ++i) {
        double error = FieldTools::spaldingYPlus(uplus[i]) - wall_distance[i] / spalding_yplus;
        REQUIRE(std::abs(error) < 1e-6);
    }
}

TEST_CASE("FieldTools binary operators") {
    VectorFieldAdapter f1("field1", FieldAttributes::Node(), 2, 3);
    VectorFieldAdapter f2("field2", FieldAttributes::Node(), 2, 3);
    for (int i = 0; i < 3; ++i) {
        f1.getVector()[0 + i * 2] = 1.0 + i;
        f1.getVector()[1 + i * 2] = 2.0 + i;

        f2.getVector()[0 + i * 2] = 1.1 + i;
        f2.getVector()[1 + i * 2] = 2.2 + i;
    }

#define CHECK_BINARY_OP(func, op)                                                       \
    {                                                                                   \
        auto result = func(f1, f2);                                                     \
        for (int i = 0; i < 3; ++i) {                                                   \
            INFO(#func);                                                                \
            auto actual = (result)->at(i);                                              \
            for (int j = 0; j < 2; ++j) {                                               \
                auto expected = f1.getVector()[j + i * 2] op f2.getVector()[j + i * 2]; \
                REQUIRE(expected == Approx(actual[j]));                                 \
            }                                                                           \
        }                                                                               \
    }
    CHECK_BINARY_OP(FieldTools::diff, -);
    CHECK_BINARY_OP(FieldTools::sum, +);
    CHECK_BINARY_OP(FieldTools::multiply, *);
    CHECK_BINARY_OP(FieldTools::divide, /);
}

TEST_CASE("FieldTools scalar operators") {
    VectorFieldAdapter f("field", FieldAttributes::Node(), 2, 3);
    for (int i = 0; i < 3; ++i) {
        f.getVector()[0 + i * 2] = 1.0 + i;
        f.getVector()[1 + i * 2] = 2.0 + i;
    }
    double scalar = 1.25;

#define CHECK_SCALAR_OP(func, op)                                   \
    {                                                               \
        auto result = func(f, scalar);                              \
        for (int i = 0; i < 3; ++i) {                               \
            INFO(#func);                                            \
            auto actual = (result)->at(i);                          \
            for (int j = 0; j < 2; ++j) {                           \
                auto expected = f.getVector()[j + i * 2] op scalar; \
                REQUIRE(expected == Approx(actual[j]));             \
            }                                                       \
        }                                                           \
    }
    CHECK_SCALAR_OP(FieldTools::diff, -);
    CHECK_SCALAR_OP(FieldTools::sum, +);
    CHECK_SCALAR_OP(FieldTools::multiply, *);
    CHECK_SCALAR_OP(FieldTools::divide, /);
}

TEST_CASE("Average node to cell exact for constant function") {
    auto mesh = inf::CartMesh::create(5, 5, 5);
    std::vector<double> test_field(mesh->nodeCount() * 5, 2.2);
    auto node_field = std::make_shared<inf::VectorFieldAdapter>(
        "twopointtwo", inf::FieldAttributes::Node(), 5, test_field);
    auto cell_field = FieldTools::nodeToCell(*mesh, *node_field);
    REQUIRE(cell_field->size() == mesh->cellCount());
    REQUIRE(cell_field->association() == FieldAttributes::Cell());
    REQUIRE(cell_field->blockSize() == 5);
    std::vector<double> temp(5);
    for (int c = 0; c < mesh->cellCount(); c++) {
        cell_field->value(c, temp.data());
        for (int e = 0; e < 5; e++) {
            REQUIRE(temp[e] == Approx(2.2));
        }
    }
}

TEST_CASE("Average cell to node exact for constant function") {
    auto mesh = inf::CartMesh::create(5, 5, 5);
    std::vector<double> test_field(mesh->cellCount() * 5, 2.2);
    auto cell_field = std::make_shared<inf::VectorFieldAdapter>(
        "twopointtwo", inf::FieldAttributes::Cell(), 5, test_field);
    auto node_field = FieldTools::cellToNode_simpleAverage(*mesh, *cell_field);
    REQUIRE(node_field->size() == mesh->nodeCount());
    REQUIRE(node_field->blockSize() == 5);
    REQUIRE(node_field->association() == FieldAttributes::Node());
    std::vector<double> temp(5);
    for (int n = 0; n < mesh->nodeCount(); n++) {
        node_field->value(n, temp.data());
        for (int e = 0; e < 5; e++) {
            REQUIRE(temp[e] == Approx(2.2));
        }
    }
}

TEST_CASE("Parallel min/max of field") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 5, 5, 5);
    SECTION("at nodes") {
        MDVector<double, 2> test_field({mesh->nodeCount(), 2});
        for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
            test_field(node_id, 0) = mesh->globalNodeId(node_id);
            test_field(node_id, 1) = 2 * mesh->globalNodeId(node_id);
        }
        VectorFieldAdapter field("field", FieldAttributes::Node(), 2, test_field.vec);
        auto min = FieldTools::findMin(mp, *mesh, field);
        REQUIRE(0.0 == min[0]);
        REQUIRE(0.0 == min[1]);

        auto node_count = globalNodeCount(mp, *mesh);
        auto max = FieldTools::findMax(mp, *mesh, field);
        REQUIRE(double(node_count - 1) == max[0]);
        REQUIRE(double(2 * (node_count - 1)) == max[1]);
    }
    SECTION("at cells") {
        MDVector<double, 2> test_field({mesh->cellCount(), 2});
        for (int cell_id = 0; cell_id < mesh->cellCount(); ++cell_id) {
            test_field(cell_id, 0) = mesh->globalCellId(cell_id);
            test_field(cell_id, 1) = 2 * mesh->globalCellId(cell_id);
        }
        VectorFieldAdapter field("field", FieldAttributes::Cell(), 2, test_field.vec);
        auto min = FieldTools::findMin(mp, *mesh, field);
        REQUIRE(0.0 == min[0]);
        REQUIRE(0.0 == min[1]);

        auto cell_count = globalCellCount(mp, *mesh);
        auto max = FieldTools::findMax(mp, *mesh, field);
        REQUIRE(double(cell_count - 1) == max[0]);
        REQUIRE(double(2 * (cell_count - 1)) == max[1]);
    }
}

TEST_CASE("rename a field") {
    constexpr int field_size = 5;
    constexpr int block_size = 2;
    std::vector<double> test_field(field_size * block_size, 2.2);

    inf::VectorFieldAdapter original(
        "original", inf::FieldAttributes::Cell(), block_size, test_field);
    auto renamed = FieldTools::rename("renamed", original);
    REQUIRE("renamed" == renamed->name());
    REQUIRE(field_size == renamed->size());
    REQUIRE(block_size == renamed->blockSize());
    for (int i = 0; i < field_size; ++i) {
        for (int j = 0; j < block_size; ++j) {
            REQUIRE(2.2 == renamed->getDouble(i, j));
        }
    }
}
TEST_CASE("Merge scalar fields") {
    constexpr int field_size = 5;
    std::vector<double> test_field(field_size, 2.2);
    auto one = std::make_shared<inf::VectorFieldAdapter>(
        "one", inf::FieldAttributes::Cell(), std::vector<double>(field_size, 1.0));
    auto two = std::make_shared<inf::VectorFieldAdapter>(
        "two", inf::FieldAttributes::Cell(), std::vector<double>(field_size, 2.0));
    auto three = std::make_shared<inf::VectorFieldAdapter>(
        "three", inf::FieldAttributes::Cell(), std::vector<double>(field_size, 3.0));

    auto one_two_three = inf::FieldTools::merge({one, two, three}, "one_two_three");
    REQUIRE(one_two_three->association() == inf::FieldAttributes::Cell());
    REQUIRE(one_two_three->blockSize() == 3);
    REQUIRE(one_two_three->size() == field_size);
    for (int i = 0; i < field_size; i++) {
        REQUIRE(one_two_three->getDouble(i, 0) == 1.0);
        REQUIRE(one_two_three->getDouble(i, 1) == 2.0);
        REQUIRE(one_two_three->getDouble(i, 2) == 3.0);
    }
}
