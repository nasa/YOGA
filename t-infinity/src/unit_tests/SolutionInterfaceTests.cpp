#include <tinf_solution.h>
#include <t-infinity/CSolutionFieldExtractor.h>
#include <t-infinity/FieldInterface.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <RingAssertions.h>

class MockSolution {
  public:
    const int n = 6;
    const char* outputs[6] = {
        "Density", "X-Momentum", "Y-Momentum", "Z-Momentum", "Energy-Per-Unit-Volume", "Turbulence-Intensity"};
    enum TINF_DATA_TYPE dtypes[6] = {TINF_DOUBLE, TINF_DOUBLE, TINF_DOUBLE, TINF_DOUBLE, TINF_DOUBLE, TINF_DOUBLE};
};

int32_t tinf_solution_get_nodal_output_names(void* const solution_instance,
                                             int64_t* const n_outputs,
                                             const char** names[],
                                             const enum TINF_DATA_TYPE* datatype[]) {
    auto solution_ptr = (MockSolution*)solution_instance;
    *names = (const char**)solution_ptr->outputs;
    *n_outputs = solution_ptr->n;
    *datatype = solution_ptr->dtypes;
    return TINF_SUCCESS;
}

int32_t tinf_solution_get_outputs_at_nodes(void* const solution_instance,
                                        const enum TINF_DATA_TYPE data_type,
                                        const int64_t start_node,
                                        const int64_t node_count,
                                        const int64_t value_count,
                                        const char* names[],
                                        void* const values) {
    if (node_count != 1) {
        return TINF_SUCCESS;
    }
    int32_t err = TINF_SUCCESS;
    std::string requested_name = names[0];
    double* ptr = (double*)values;
    if ("Density" == requested_name) {
        *ptr = 1337.0;
    } else if ("X-Momentum" == requested_name) {
        *ptr = 1.0;
    } else if ("Y-Momentum" == requested_name) {
        *ptr = 2.0;
    } else if ("Z-Momentum" == requested_name) {
        *ptr = 3.0;
    } else if ("Energy-Per-Unit-Volume" == requested_name) {
        *ptr = 4.0;
    } else if ("Turbulence-Intensity" == requested_name) {
        *ptr = 9001.5;
    } else {
        err = TINF_FAILURE;
    }
    return err;
}

TEST_CASE("Can get names") {
    auto mock_solution_ptr = new MockSolution();
    int64_t n_names = 0;
    int32_t err = 0;
    char** names;
    TINF_DATA_TYPE* dtype = nullptr;
    err = tinf_solution_get_nodal_output_names(
        mock_solution_ptr, &n_names, (const char***)&names, (const enum TINF_DATA_TYPE**)&dtype);
    REQUIRE(err == TINF_SUCCESS);
    REQUIRE(n_names == 6);
    REQUIRE("Density" == std::string(names[0]));
    REQUIRE("X-Momentum" == std::string(names[1]));
    REQUIRE("Y-Momentum" == std::string(names[2]));
    REQUIRE("Z-Momentum" == std::string(names[3]));
    REQUIRE("Energy-Per-Unit-Volume" == std::string(names[4]));
    REQUIRE("Turbulence-Intensity" == std::string(names[5]));

    delete mock_solution_ptr;
}

TEST_CASE("Can get values") {
    auto mock_solution_ptr = new MockSolution();
    double d = 0.0;
    int32_t err;
    const char* name[1] = {"Density"};
    err = tinf_solution_get_outputs_at_nodes(mock_solution_ptr, TINF_DOUBLE, 0, 1, 1, name, &d);
    REQUIRE(d == 1337.0);
    REQUIRE(err == TINF_SUCCESS);

    const char* name2[1] = {"Turbulence-Intensity"};
    err = tinf_solution_get_outputs_at_nodes(mock_solution_ptr, TINF_DOUBLE, 0, 1, 1, name2, &d);
    REQUIRE(d == 9001.5);
    REQUIRE(err == TINF_SUCCESS);

    delete mock_solution_ptr;
}

TEST_CASE("Can make field interface") {
    auto mock_solution_ptr = new MockSolution();
    auto fields = extractFromCSolution(mock_solution_ptr, 1);
    REQUIRE(fields.size() == 6);
    delete mock_solution_ptr;
}
