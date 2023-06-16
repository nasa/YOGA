#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include "c_grouping_pre_processor.h"
#include <array>
#include <t-infinity/PancakeMeshAdapterTo.h>
#include <t-infinity/Mangle.h>

template <typename T>
size_t countUnique(const std::vector<T>& vec) {
    std::set<int64_t> unique(vec.begin(), vec.end());
    return unique.size();
}

size_t countUniqueNodesInCell(void* mesh_handle, int cell_length, int cell_id) {
    std::vector<int64_t> cell_nodes(cell_length);
    mangle(tinf_mesh_element_nodes)(mesh_handle, cell_id, cell_nodes.data());
    return countUnique(cell_nodes);
}

TEST_CASE("dog") {
    void* mesh_handle;
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto comm = mp.getCommunicator();
    std::string test_grid = GRIDS_DIR;
    test_grid += "lb8.ugrid/1-cell.lb8.ugrid";
    auto error = tinf_preprocess(&mesh_handle, &comm, test_grid.c_str());
    if (error != TINF_SUCCESS) {
        printf("Error detected preprocessing the mesh");
    }

    SECTION("can read node count") {
        int64_t node_count = mangle(tinf_mesh_node_count)(mesh_handle, &error);
        REQUIRE(node_count == 8);
    }
    SECTION("resident(owned) node count") {
        int64_t node_count = mangle(tinf_mesh_resident_node_count)(mesh_handle, &error);
        REQUIRE(node_count == 8);
    }

    SECTION("get cell count") {
        int64_t cell_count = mangle(tinf_mesh_element_count)(mesh_handle, &error);
        REQUIRE(cell_count == 7);
    }

    SECTION("get element type count") {
        int64_t cell_count = mangle(tinf_mesh_element_type_count)(mesh_handle, TINF_TETRA_4, &error);
        REQUIRE(cell_count == 0);

        cell_count = mangle(tinf_mesh_element_type_count)(mesh_handle, TINF_HEXA_8, &error);
        REQUIRE(cell_count == 1);

        cell_count = mangle(tinf_mesh_element_type_count)(mesh_handle, TINF_QUAD_4, &error);
        REQUIRE(cell_count == 6);
    }

    SECTION("get nodes") {
        std::array<double, 2> x, y, z;
        auto err = mangle(tinf_mesh_nodes_coordinates)(
            mesh_handle, TINF_DOUBLE, 0, 2, (void*)x.data(), (void*)y.data(), (void*)z.data());
        REQUIRE(err == TINF_SUCCESS);
        REQUIRE(x[0] == 0.0);
        REQUIRE(y[0] == 0.0);
        REQUIRE(z[0] == 0.0);

        REQUIRE(x[1] == 1.0);
        REQUIRE(y[1] == 0.0);
        REQUIRE(z[1] == 0.0);
    }

    SECTION("get complex error") {
        std::array<std::complex<double>, 2> x, y, z;
        auto error = mangle(tinf_mesh_nodes_coordinates)(
            mesh_handle, TINF_CMPLX_DOUBLE, 0, 2, (void*)x.data(), (void*)y.data(), (void*)z.data());
        REQUIRE(error == TINF_SUCCESS);
        REQUIRE(x[0].real() == 0.0);
        REQUIRE(y[0].real() == 0.0);
        REQUIRE(z[0].real() == 0.0);
        REQUIRE(x[0].imag() == 0.0);
        REQUIRE(y[0].imag() == 0.0);
        REQUIRE(z[0].imag() == 0.0);

        REQUIRE(x[1].real() == 1.0);
        REQUIRE(y[1].real() == 0.0);
        REQUIRE(z[1].real() == 0.0);
        REQUIRE(x[1].imag() == 0.0);
        REQUIRE(y[1].imag() == 0.0);
        REQUIRE(z[1].imag() == 0.0);
    }

    SECTION("get element type") {
        TINF_ELEMENT_TYPE element_type;
        for (int c = 0; c < 6; c++) {
            element_type = mangle(tinf_mesh_element_type)(mesh_handle, c, &error);
            REQUIRE(element_type == TINF_QUAD_4);
        }
        element_type = mangle(tinf_mesh_element_type)(mesh_handle, 6, &error);
        REQUIRE(element_type == TINF_HEXA_8);
    }

    SECTION("get element nodes") {
        REQUIRE(countUniqueNodesInCell(mesh_handle, 4, 0) == 4);
        REQUIRE(countUniqueNodesInCell(mesh_handle, 8, 6) == 8);
    }

    SECTION("get global node id") {
        std::vector<int64_t> gids;
        for (int c = 0; c < 8; c++) {
            int64_t g = mangle(tinf_mesh_global_node_id)(mesh_handle, c, &error);
            gids.push_back(g);
        }
        REQUIRE(countUnique(gids) == 8);
    }

    SECTION("get global cell id") {
        std::vector<int64_t> gids;
        for (int c = 0; c < 7; c++) {
            int64_t g;
            g = mangle(tinf_mesh_global_element_id)(mesh_handle, c, &error);
            gids.push_back(g);
        }
        REQUIRE(countUnique(gids) == 7);
    }

    SECTION("tag is valid") {
        for (int c = 0; c < 7; c++) {
            int64_t t = mangle(tinf_mesh_element_tag)(mesh_handle, c, &error);
            REQUIRE(t > -1e99);
        }
    }

    SECTION("get element owner") {
        for (int c = 0; c < 7; c++) {
            int64_t owner = mangle(tinf_mesh_element_owner)(mesh_handle, c, &error);
            REQUIRE(owner == 0);
        }
    }

    SECTION("get node owner") {
        for (int c = 0; c < 8; c++) {
            int64_t owner = mangle(tinf_mesh_node_owner)(mesh_handle, c, &error);
            REQUIRE(owner == 0);
        }
    }

    SECTION("get partition id") {
        int64_t pid = mangle(tinf_mesh_partition_id)(mesh_handle, &error);
        REQUIRE(pid == mp.Rank());
    }

    tinf_mesh_destroy(&mesh_handle);
}
