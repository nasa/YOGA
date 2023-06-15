#include <RingAssertions.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/MapBetweenMeshes.h>

TEST_CASE("Map between meshes nodes, same partition just reordered") {
    std::vector<Parfait::Point<double>> points(5);
    inf::TinfMeshData data_from;
    data_from.global_node_id = {7, 6, 5, 4, 3};
    data_from.points = points;
    inf::TinfMeshData data_to;
    data_to.global_node_id = {3, 4, 5, 6, 7};
    data_to.points = points;

    inf::TinfMesh from(data_from, 0);
    inf::TinfMesh to(data_to, 0);

    auto node_map = inf::localToLocalNodes(from, to);
    REQUIRE(node_map.size() == 5);
    REQUIRE(node_map.at(0) == 4);
    REQUIRE(node_map.at(1) == 3);
    REQUIRE(node_map.at(2) == 2);
    REQUIRE(node_map.at(3) == 1);
    REQUIRE(node_map.at(4) == 0);
}

TEST_CASE("Map between meshes nodes, from partition is bigger") {
    std::vector<Parfait::Point<double>> points(5);
    inf::TinfMeshData data_to;
    data_to.global_node_id = {3, 4, 5, 6, 7};
    data_to.points = points;

    points.push_back(Parfait::Point<double>{0, 0, 0});
    inf::TinfMeshData data_from;
    data_from.global_node_id = {7, 6, 5, 4, 3, 2};
    data_from.points = points;

    inf::TinfMesh from(data_from, 0);
    inf::TinfMesh to(data_to, 0);

    auto node_map = inf::localToLocalNodes(from, to);
    REQUIRE(node_map.size() == 5);
    REQUIRE(node_map.at(0) == 4);
    REQUIRE(node_map.at(1) == 3);
    REQUIRE(node_map.at(2) == 2);
    REQUIRE(node_map.at(3) == 1);
    REQUIRE(node_map.at(4) == 0);
}

TEST_CASE("Map between meshes nodes, to partition is bigger") {
    std::vector<Parfait::Point<double>> points(5);
    inf::TinfMeshData data_from;
    data_from.global_node_id = {7, 6, 5, 4, 3};
    data_from.points = points;

    points.push_back(Parfait::Point<double>{0, 0, 0});
    inf::TinfMeshData data_to;
    data_to.global_node_id = {3, 4, 5, 6, 7, 2};
    data_to.points = points;

    inf::TinfMesh from(data_from, 0);
    inf::TinfMesh to(data_to, 0);

    auto node_map = inf::localToLocalNodes(from, to);
    REQUIRE(node_map.size() == 5);
    REQUIRE(node_map.at(0) == 4);
    REQUIRE(node_map.at(1) == 3);
    REQUIRE(node_map.at(2) == 2);
    REQUIRE(node_map.at(3) == 1);
    REQUIRE(node_map.at(4) == 0);
}
