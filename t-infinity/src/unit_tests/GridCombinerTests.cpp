#include <t-infinity/CompositeMeshBuilder.h>
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/MeshHelpers.h>
#include <RingAssertions.h>
#include "MockMeshes.h"

using namespace inf;

TEST_CASE("generate new global node ids for a composite of several grids") {
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<std::shared_ptr<inf::MeshInterface>> meshes;
    meshes.push_back(mock::getSingleTetMesh());
    meshes.push_back(mock::getSingleTetMesh());

    auto super_mesh = inf::CompositeMeshBuilder::createCompositeMesh(mp, meshes);
    REQUIRE(globalNodeCount(mp, *super_mesh) == 8);
    REQUIRE(globalCellCount(mp, *super_mesh) == 10);
}
