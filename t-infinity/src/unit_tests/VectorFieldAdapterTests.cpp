#include <t-infinity/VectorFieldAdapter.h>
#include <RingAssertions.h>
#include <memory>
#include <vector>
using namespace inf;

TEST_CASE("Vector field adapter for double scalar") {
    std::vector<double> scalar_field(10, 6.7);
    auto field = std::make_shared<VectorFieldAdapter>(
        "scalar", FieldAttributes::Unassigned(), 1, scalar_field);
    REQUIRE(field->name() == "scalar");
    REQUIRE(field->size() == 10);
    REQUIRE(field->blockSize() == 1);
    double p;
    field->value(5, &p);
    REQUIRE(p == 6.7);
}

TEST_CASE("Vector field adapter for double 2 length vector") {
    std::vector<double> scalar_field(10, 6.7);
    constexpr int entry_length = 2;
    SECTION("Provide full std::vector") {
        auto field = std::make_shared<VectorFieldAdapter>(
            "scalar", FieldAttributes::Unassigned(), entry_length, scalar_field);
        REQUIRE(field->name() == "scalar");
        REQUIRE(field->size() == 5);
        REQUIRE(field->blockSize() == entry_length);
        std::array<double, entry_length> p{};
        field->value(4, p.data());
        REQUIRE(p.at(0) == 6.7);
        REQUIRE(p.at(1) == 6.7);
    }
    SECTION("Provide size and entry_length") {
        const int field_size = scalar_field.size() / entry_length;
        auto field = std::make_shared<VectorFieldAdapter>(
            "scalar", FieldAttributes::Unassigned(), entry_length, field_size);
        field->getVector() = scalar_field;
        REQUIRE(field->name() == "scalar");
        REQUIRE(field->size() == 5);
        REQUIRE(field->blockSize() == 2);
        std::array<double, entry_length> p{};
        field->value(4, p.data());
        REQUIRE(p.at(0) == 6.7);
        REQUIRE(p.at(1) == 6.7);
    }
}

TEST_CASE("Create field from pointer and length") {
    std::vector<std::array<double, 9>> tensor_field(10, {1, 1, 1, 2, 2, 2, 3, 3, 3});
    auto field = std::make_shared<VectorFieldAdapter>("tensor",
                                                      FieldAttributes::Unassigned(),
                                                      9,
                                                      (const double*)tensor_field.data(),
                                                      tensor_field.size());
    REQUIRE(field->name() == "tensor");
    REQUIRE(field->size() == 10);
    REQUIRE(field->blockSize() == 9);
    std::array<double, 9> m;
    field->value(2, m.data());
    REQUIRE(m[0] == 1);
    REQUIRE(m[3] == 2);
    REQUIRE(m[6] == 3);
}

TEST_CASE("Create a blocksize 12 field") {
    int blocksize = 12;
    int num_nodes = 10;
    // somehow pack the vector
    std::vector<double> my_data(num_nodes * blocksize);
    for (int n = 0; n < num_nodes; n++) {
        for (int i = 0; i < blocksize; i++) {
            my_data[blocksize * n + i] = 42 * n;
        }
    }

    auto field = std::make_shared<inf::VectorFieldAdapter>(
        "my-test-field", inf::FieldAttributes::Node(), blocksize, my_data);
    REQUIRE(field->size() == num_nodes);
    REQUIRE(field->blockSize() == 12);

    std::array<double, 12> temp_array;
    int test_node_id = 7;
    field->value(test_node_id, temp_array.data());
    for (int i = 0; i < 12; i++) REQUIRE(temp_array[i] == 42 * test_node_id);
}

TEST_CASE("Vector must be able to answer what its association is") {
    std::vector<double> scalar_field(10, 6.7);
    auto field = std::make_shared<VectorFieldAdapter>(
        "scalar", inf::FieldAttributes::Node(), 2, scalar_field);
    REQUIRE(field->name() == "scalar");
    REQUIRE(field->size() == 5);
    REQUIRE(field->blockSize() == 2);
    std::array<double, 2> p;
    field->value(4, p.data());
    REQUIRE(p[0] == 6.7);
    auto a = field->association();
    REQUIRE(inf::FieldAttributes::Node() == a);
}
