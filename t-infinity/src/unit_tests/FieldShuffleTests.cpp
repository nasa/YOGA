#include <RingAssertions.h>
#include <utility>
#include "MockMeshes.h"
#include <t-infinity/GlobalToLocal.h>
#include <t-infinity/FieldShuffle.h>

using namespace inf;

TEST_CASE("Transfer field from different meshes") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto tet_mesh1 = mock::getSingleTetMesh(0);
    auto tet_mesh2 = mock::getSingleTetMesh(0);
    auto sync_pattern = FieldShuffle::buildMeshToMeshNodeSyncPattern(mp, *tet_mesh1, *tet_mesh2);
    std::vector<double> mesh1_values = {0, 1, 2, 3};
    std::vector<double> mesh2_values(tet_mesh2->nodeCount(), -1);

    std::map<long, int> mesh2_g2l;
    mesh2_g2l[3] = 0;
    mesh2_g2l[2] = 1;
    mesh2_g2l[1] = 2;
    mesh2_g2l[0] = 3;
    auto field = FieldShuffle::FieldMeshToMeshWrapper<double>(
        mesh1_values, mesh2_values, GlobalToLocal::buildNode(*tet_mesh1), mesh2_g2l, 1);
    auto syncer = buildMeshToMeshFieldSyncer(mp, field, sync_pattern);
    syncer.start();
    syncer.finish();

    REQUIRE(3.0 == mesh2_values[0]);
    REQUIRE(2.0 == mesh2_values[1]);
    REQUIRE(1.0 == mesh2_values[2]);
    REQUIRE(0.0 == mesh2_values[3]);
}
TEST_CASE("Transfer field from different meshes - strided") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto tet_mesh1 = mock::getSingleTetMesh(0);
    auto tet_mesh2 = mock::getSingleTetMesh(0);
    auto sync_pattern = FieldShuffle::buildMeshToMeshNodeSyncPattern(mp, *tet_mesh1, *tet_mesh2);
    std::vector<double> mesh1_values = {0, 0.1, 1, 1.1, 2, 2.1, 3, 3.1};
    std::vector<double> mesh2_values(2 * tet_mesh2->nodeCount(), -1);

    int stride = 2;
    auto mesh1_g2l = GlobalToLocal::buildNode(*tet_mesh1);
    std::map<long, int> mesh2_g2l;
    mesh2_g2l[3] = 0;
    mesh2_g2l[2] = 1;
    mesh2_g2l[1] = 2;
    mesh2_g2l[0] = 3;
    auto syncer = FieldShuffle::buildMeshToMeshFieldStridedSyncer(
        mp, mesh1_values.data(), mesh2_values.data(), sync_pattern, mesh1_g2l, mesh2_g2l, stride);
    syncer.start();
    syncer.finish();

    REQUIRE(3.0 == mesh2_values[0]);
    REQUIRE(3.1 == mesh2_values[1]);
    REQUIRE(2.0 == mesh2_values[2]);
    REQUIRE(2.1 == mesh2_values[3]);
    REQUIRE(1.0 == mesh2_values[4]);
    REQUIRE(1.1 == mesh2_values[5]);
    REQUIRE(0.0 == mesh2_values[6]);
    REQUIRE(0.1 == mesh2_values[7]);
}