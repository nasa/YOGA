#include <RingAssertions.h>
#include "MockMeshes.h"
#include <t-infinity/CartMesh.h>

TEST_CASE("Mesh interface can get cell nodes by resizing a vector"){
    auto mesh = inf::mock::getSingleTetMesh();
    std::vector<int> cell_nodes;
    mesh->cell(0, cell_nodes);
    REQUIRE(cell_nodes.size() == 3);
}

TEST_CASE("Mesh interface can supply cell/node ownership") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 5, 5, 5);
    for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
        bool owned_node = mesh->ownedNode(node_id);
        REQUIRE(owned_node == mesh->ownedNode(node_id));
    }
    for (int cell_id = 0; cell_id < mesh->cellCount(); ++cell_id) {
        bool owned_cell = mesh->ownedCell(cell_id);
        REQUIRE(owned_cell == mesh->ownedCell(cell_id));
    }
}
