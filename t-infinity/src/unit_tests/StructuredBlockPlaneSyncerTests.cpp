#include <RingAssertions.h>
#include <t-infinity/StructuredBlockPlaneSyncer.h>

using namespace inf;

TEST_CASE("StructuredBlockPlane a block to obtain a plane of data") {
    Vector3D<int> neighbor_block({3, 4, 5});
    int index = 0;
    for (int i = 0; i < neighbor_block.extent(0); ++i) {
        for (int j = 0; j < neighbor_block.extent(1); ++j) {
            for (int k = 0; k < neighbor_block.extent(2); ++k) {
                neighbor_block(i, j, k) = index++;
            }
        }
    }
    auto slice = StructuredBlockPlane<int>(neighbor_block, neighbor_block.dimensions(), K, 4);
    REQUIRE(neighbor_block.dimensions() == slice.original_block_dimensions);
    REQUIRE(3 == slice.plane.extent(0));
    REQUIRE(4 == slice.plane.extent(1));
    for (int i = 0; i < neighbor_block.extent(0); ++i) {
        for (int j = 0; j < neighbor_block.extent(1); ++j) {
            REQUIRE(neighbor_block(i, j, 4) == slice(i, j, 4));
        }
    }
}

TEST_CASE("Sync planes") {
    MessagePasser mp(MPI_COMM_WORLD);
    MeshConnectivity mesh_connectivity;
    std::map<int, std::array<int, 3>> block_dimensions;
    if (mp.Rank() == 0) {
        BlockFaceConnectivity& block_face_connectivity = mesh_connectivity[0];
        block_face_connectivity[IMAX].neighbor_side = IMIN;
        for (auto axis : {I, J, K}) block_face_connectivity[IMAX].face_mapping[axis] = {axis, SameDirection};
        block_face_connectivity[IMAX].neighbor_block_id = 1;
        block_dimensions[0] = {3, 4, 5};
    }
    if (mp.Rank() == mp.NumberOfProcesses() - 1) {
        BlockFaceConnectivity& block_face_connectivity = mesh_connectivity[1];
        block_face_connectivity[IMIN].neighbor_side = IMAX;
        for (auto axis : {I, J, K}) block_face_connectivity[IMIN].face_mapping[axis] = {axis, SameDirection};
        block_face_connectivity[IMIN].neighbor_block_id = 0;
        block_dimensions[1] = {3, 4, 5};
    }

    auto syncer = StructuredBlockPlaneSyncer(
        mp, mesh_connectivity, block_dimensions, 1, inf::StructuredBlockPlaneSyncer::Node);

    auto get_value = [&](int block_id, int i, int j, int k) { return std::array<int, 2>{block_id, i}; };
    auto set_value = [&](int block_id, int i, int j, int k, const std::array<int, 2>& v) {
        int expected_block_id = block_id == 0 ? 1 : 0;
        REQUIRE(expected_block_id == v.front());
        int expected_i = 1;
        REQUIRE(expected_i == v.back());
    };
    syncer.sync(get_value, set_value, 0);
}
