#include <RingAssertions.h>
#include <t-infinity/MeshExploder.h>
#include <t-infinity/MeshSanityChecker.h>
#include <t-infinity/Shortcuts.h>
#include "MockMeshes.h"

TEST_CASE("Can explode a single tet mesh") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::mock::getSingleTetMesh();

    REQUIRE(mesh->nodeCount() == 4);

    auto exploded_mesh = inf::explodeMesh(mp, mesh);

    REQUIRE(exploded_mesh->cellCount() == mesh->cellCount());
    REQUIRE(exploded_mesh->nodeCount() == 16);

    REQUIRE(exploded_mesh->mesh.global_node_id.size() == 16);
    REQUIRE(exploded_mesh->mesh.node_owner.size() == 16);

    for (long i = 0; i < 16; i++) {
        REQUIRE(exploded_mesh->mesh.global_node_id[i] == i);
    }
}

TEST_CASE("Can explode a single triangle mesh") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = std::make_shared<inf::TinfMesh>(inf::mock::triangleWithBar(), 0);

    REQUIRE(mesh->nodeCount() == 3);

    auto exploded_mesh = inf::explodeMesh(mp, mesh);

    REQUIRE(exploded_mesh->cellCount() == mesh->cellCount());
    REQUIRE(exploded_mesh->nodeCount() == 5);

    REQUIRE(exploded_mesh->mesh.global_node_id.size() == 5);
    REQUIRE(exploded_mesh->mesh.node_owner.size() == 5);

    for (long i = 0; i < 5; i++) {
        REQUIRE(exploded_mesh->mesh.global_node_id[i] == i);
    }
}
