#include <RingAssertions.h>
#include <t-infinity/TinfMesh.h>
#include <parfait/EdgeBuilder.h>
#include <parfait/CGNSElements.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/CartMesh.h>

template <class T>
bool contains(const T& container, int value) {
    return std::find(container.begin(), container.end(), value) != container.end();
}

TEST_CASE("Build n2n graph from mesh") {
    inf::TinfMeshData mesh_data;
    mesh_data.points.resize(5);
    mesh_data.cells[inf::MeshInterface::TETRA_4] = {0, 1, 2, 3, 2, 1, 0, 4};

    int mock_partition_id = 7;
    inf::TinfMesh mesh(mesh_data, mock_partition_id);

    auto n2n = inf::NodeToNode::build(mesh);
    REQUIRE(5 == n2n.size());
    REQUIRE(4 == n2n[0].size());
    REQUIRE(contains(n2n[0], 1));
    REQUIRE(contains(n2n[0], 2));
    REQUIRE(contains(n2n[0], 3));
    REQUIRE(contains(n2n[0], 4));

    REQUIRE(4 == n2n[1].size());
    REQUIRE(contains(n2n[1], 0));
    REQUIRE(contains(n2n[1], 2));
    REQUIRE(contains(n2n[1], 3));
    REQUIRE(contains(n2n[1], 4));

    REQUIRE(4 == n2n[2].size());
    REQUIRE(contains(n2n[2], 0));
    REQUIRE(contains(n2n[2], 1));
    REQUIRE(contains(n2n[2], 3));
    REQUIRE(contains(n2n[2], 4));

    REQUIRE(3 == n2n[3].size());
    REQUIRE(contains(n2n[3], 0));
    REQUIRE(contains(n2n[3], 1));
    REQUIRE(contains(n2n[3], 2));

    REQUIRE(3 == n2n[4].size());
    REQUIRE(contains(n2n[4], 0));
    REQUIRE(contains(n2n[4], 1));
    REQUIRE(contains(n2n[4], 2));
}

TEST_CASE("Build n2n graph on surface mesh") {
    auto mesh = inf::CartMesh::create(2, 2, 2);

    auto n2n = inf::NodeToNode::buildSurfaceOnly(*mesh);
    REQUIRE(27 == n2n.size());
    int zero_neighbor_count = 0;
    for(auto & neighbors : n2n){
        if(neighbors.size() == 0)
            zero_neighbor_count++;
    }
    // just the middle node should have no neighbors.
    REQUIRE(1 == zero_neighbor_count);
}
