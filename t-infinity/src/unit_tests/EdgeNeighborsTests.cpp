#include <RingAssertions.h>
#include <t-infinity/MeshConnectivity.h>
#include "MockMeshes.h"

template <typename T>
bool contains(const std::vector<T>& v, const T& t) {
    for (auto o : v) {
        if (o == t) return true;
    }
    return false;
}

template <typename T>
bool contains(const std::vector<T>& v, const std::vector<T>& list) {
    for (auto l : list) {
        if (not contains(v, l)) return false;
    }
    return true;
}

TEST_CASE("Can build edge-to-cell") {
    auto mesh_data = inf::mock::twoTouchingTets();
    auto two_tets = std::make_shared<inf::TinfMesh>(mesh_data, 0);
    auto edges = inf::EdgeToNode::build(*two_tets);
    auto edge_to_cell = inf::EdgeToCell::build(*two_tets, edges);

    REQUIRE(edge_to_cell.size() == edges.size());
    REQUIRE(edge_to_cell[0].size() == 1);
    REQUIRE(edge_to_cell[1].size() == 1);
    REQUIRE(edge_to_cell[2].size() == 1);
    REQUIRE(edge_to_cell[3].size() == 2);
    REQUIRE(edge_to_cell[4].size() == 2);
    REQUIRE(edge_to_cell[5].size() == 1);
    REQUIRE(edge_to_cell[6].size() == 2);
    REQUIRE(edge_to_cell[7].size() == 1);
    REQUIRE(edge_to_cell[8].size() == 1);
}

TEST_CASE("Can build edge-to-cell nodes") {
    auto mesh_data = inf::mock::twoTouchingTets();
    auto two_tets = std::make_shared<inf::TinfMesh>(mesh_data, 0);
    auto edges = inf::EdgeToNode::build(*two_tets);
    auto edge_to_cell = inf::EdgeToCellNodes::build(*two_tets, edges);

    REQUIRE(edge_to_cell.size() == edges.size());
    REQUIRE(edge_to_cell[0].size() == 4);
    REQUIRE(edge_to_cell[1].size() == 4);
    REQUIRE(edge_to_cell[2].size() == 4);
    REQUIRE(edge_to_cell[3].size() == 5);
    REQUIRE(edge_to_cell[4].size() == 5);
    REQUIRE(edge_to_cell[5].size() == 4);
    REQUIRE(edge_to_cell[6].size() == 5);
    REQUIRE(edge_to_cell[7].size() == 4);
    REQUIRE(edge_to_cell[8].size() == 4);

    std::vector<int> first_tet_nodes = {0, 1, 2, 3};
    std::vector<int> second_tet_nodes = {3, 2, 1, 4};
    std::vector<int> all_nodes = {0, 1, 2, 3, 4};
    REQUIRE(contains(edge_to_cell[0], first_tet_nodes));
    REQUIRE(contains(edge_to_cell[1], first_tet_nodes));
    REQUIRE(contains(edge_to_cell[2], first_tet_nodes));
    REQUIRE(contains(edge_to_cell[3], all_nodes));
    REQUIRE(contains(edge_to_cell[4], all_nodes));
    REQUIRE(contains(edge_to_cell[5], second_tet_nodes));
    REQUIRE(contains(edge_to_cell[6], all_nodes));
    REQUIRE(contains(edge_to_cell[7], second_tet_nodes));
    REQUIRE(contains(edge_to_cell[8], second_tet_nodes));
}