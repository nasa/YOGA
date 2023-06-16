#include <RingAssertions.h>
#include <t-infinity/StitchMesh.h>
#include <t-infinity/Debug.h>


TEST_CASE("Can stitch two meshes") {
    auto volume_mesh = inf::CartMesh::createVolume(2, 2, 2);
    auto surface_mesh = inf::CartMesh::createSurface(2, 2, 2);

    auto merged_mesh = inf::StitchMesh::stitchMeshes({volume_mesh, surface_mesh});
    REQUIRE(merged_mesh->cellCount() == 8+24);
    REQUIRE(merged_mesh->nodeCount() == 27);
    // check that face neighbors are correct
    auto bad = inf::debug::highlightCellsMissingNeighborsAsField(*merged_mesh)->getVector();
    for(int c = 0; c < merged_mesh->cellCount(); c++){
        REQUIRE(bad[c] < 0.5);
    }
}

TEST_CASE("Can stitch two meshes and not add duplicate cells") {
    auto volume_mesh1 = inf::CartMesh::createVolume(2, 1, 1);
    auto volume_mesh2 = inf::CartMesh::createVolume(2, 1, 1);

    auto merged_mesh = inf::StitchMesh::stitchMeshes({volume_mesh1, volume_mesh2});
    REQUIRE(merged_mesh->nodeCount() == 12);
    REQUIRE(merged_mesh->cellCount() == 2);
}

TEST_CASE("Can stitch two meshes and remove internal faces") {
    auto volume_mesh1 = inf::CartMesh::create(2, 2, 2, {{0, 0, 0}, {1, 1, 1}});
    auto volume_mesh2 = inf::CartMesh::create(2, 2, 2, {{1, 0, 0}, {2, 1, 1}});

    auto merged_mesh = inf::StitchMesh::stitchMeshes({volume_mesh1, volume_mesh2});
//    REQUIRE(merged_mesh->nodeCount() == 12);
    REQUIRE(merged_mesh->cellCount() == 16 + 4*10);
    auto bad = inf::debug::highlightCellsMissingNeighborsAsField(*merged_mesh)->getVector();
    for(int c = 0; c < merged_mesh->cellCount(); c++){
        REQUIRE(bad[c] < 0.5);
    }
}
