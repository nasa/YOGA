#include <RingAssertions.h>
#include <t-infinity/StructuredMeshGlobalIds.h>
#include <t-infinity/SMesh.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/StructuredMeshHelpers.h>
#include <t-infinity/StructuredToUnstructuredMeshAdapter.h>

using namespace inf;
using Point = Parfait::Point<double>;

TEST_CASE("Can assign global node ids to a StructuredMesh") {
    MessagePasser mp(MPI_COMM_WORLD);
    MeshConnectivity mesh_connectivity;
    std::map<int, std::array<int, 3>> block_dimensions;
    if (mp.Rank() == 0) {
        BlockFaceConnectivity& block_face_connectivity = mesh_connectivity[0];
        block_face_connectivity[IMAX].neighbor_side = IMIN;
        for (auto axis : {I, J, K}) block_face_connectivity[IMAX].face_mapping[axis] = {axis, SameDirection};
        block_face_connectivity[IMAX].neighbor_block_id = 27;
        block_dimensions[0] = {2, 3, 4};
    }
    if (mp.Rank() == mp.NumberOfProcesses() - 1) {
        BlockFaceConnectivity& block_face_connectivity = mesh_connectivity[27];
        block_face_connectivity[IMIN].neighbor_side = IMAX;
        for (auto axis : {I, J, K}) block_face_connectivity[IMIN].face_mapping[axis] = {axis, SameDirection};
        block_face_connectivity[IMIN].neighbor_block_id = 0;
        block_dimensions[27] = {2, 3, 4};
    }
    auto global_ids = assignStructuredMeshGlobalNodeIds(mp, mesh_connectivity, block_dimensions);
    REQUIRE(global_ids.ids.size() == size_t(2 / mp.NumberOfProcesses()));
    if (mp.Rank() == 0) {
        long expected_gid = 0;
        loopMDRange({0, 0, 0}, block_dimensions.at(0), [&](int i, int j, int k) {
            auto gid = global_ids.ids.at(0)(i, j, k);
            REQUIRE(0 == global_ids.owner.getOwningBlock(gid));
            REQUIRE(expected_gid++ == gid);
        });
    }
    if (mp.Rank() == mp.NumberOfProcesses() - 1) {
        long index = 2 * 3 * 4;
        loopMDRange({0, 0, 0}, block_dimensions.at(27), [&](int i, int j, int k) {
            INFO("i: " << i << " j: " << j << " k: " << k);
            auto gid = global_ids.ids.at(27)(i, j, k);
            int expected = i == 0 ? 0 : 27;
            REQUIRE(expected == global_ids.owner.getOwningBlock(gid));
            MDIndex<3, LayoutRight> md_index({2, 3, 4});
            long expected_gid = i == 0 ? md_index(1, j, k) : index++;
            REQUIRE(expected_gid == gid);
        });
    }
}

TEST_CASE("Can assign global cell ids to a StructuredMesh") {
    MessagePasser mp(MPI_COMM_WORLD);
    std::map<int, std::array<int, 3>> block_cell_dimensions;
    if (mp.Rank() == 0) {
        block_cell_dimensions[0] = {2, 3, 4};
    }
    if (mp.Rank() == mp.NumberOfProcesses() - 1) {
        block_cell_dimensions[27] = {2, 3, 4};
    }
    auto global_ids = assignStructuredMeshGlobalCellIds(mp, block_cell_dimensions);
    REQUIRE(global_ids.ids.size() == size_t(2 / mp.NumberOfProcesses()));
    if (mp.Rank() == 0) {
        long expected_gid = 0;
        loopMDRange({0, 0, 0}, block_cell_dimensions.at(0), [&](int i, int j, int k) {
            auto gid = global_ids.ids.at(0)(i, j, k);
            REQUIRE(0 == global_ids.owner.getOwningBlock(gid));
            REQUIRE(expected_gid++ == gid);
        });
    }
    if (mp.Rank() == mp.NumberOfProcesses() - 1) {
        long expected_gid = 2 * 3 * 4;
        loopMDRange({0, 0, 0}, block_cell_dimensions.at(27), [&](int i, int j, int k) {
            auto gid = global_ids.ids.at(27)(i, j, k);
            INFO("i: " << i << " j: " << j << " k: " << k << " global id: " << gid);
            REQUIRE(expected_gid++ == gid);
            REQUIRE(27 == global_ids.owner.getOwningBlock(gid));
        });
    }
}

TEST_CASE("Build unique global node_ids via adt") {
    MessagePasser mp(MPI_COMM_WORLD);
    INFO("Rank: " << mp.Rank());
    SMesh mesh;
    if (mp.Rank() == 0)
        mesh.add(std::make_shared<CartesianBlock>(Point{0, 0, 0}, Point{1, 1, 1}, std::array<int, 3>{1, 1, 1}, 0));
    if (mp.Rank() == mp.NumberOfProcesses() - 1)
        mesh.add(std::make_shared<CartesianBlock>(Point{1, 0, 0}, Point{2, 1, 1}, std::array<int, 3>{1, 1, 1}, 1));

    auto globals = assignUniqueGlobalNodeIds(mp, mesh);
    REQUIRE(mesh.blockIds().size() == globals.ids.size());
    if (mp.Rank() == 0) {
        long gid = 0;
        loopMDRange({0, 0, 0}, globals.ids.at(0).dimensions(), [&](int i, int j, int k) {
            REQUIRE(gid == globals.ids.at(0)(i, j, k));
            INFO("gid: " << gid);
            REQUIRE(0 == globals.owner.getOwningBlock(gid));
            REQUIRE(0 == globals.owner.getOwningRank(gid));
            gid += 1;
        });
    }
    if (mp.Rank() == mp.NumberOfProcesses() - 1) {
        long expected_gid = 4;
        loopMDRange({0, 0, 0}, globals.ids.at(1).dimensions(), [&](int i, int j, int k) {
            auto gid = globals.ids.at(1)(i, j, k);
            int owning_block = gid >= 8 ? 1 : 0;
            INFO("gid: " << gid);
            REQUIRE(expected_gid++ == gid);
            REQUIRE(owning_block == globals.owner.getOwningBlock(gid));
        });
    }
}