#include <parfait/CartBlock.h>
#include "MeshDensityEstimator.h"
#include <RingAssertions.h>

using namespace YOGA;
using namespace Parfait;

auto generateMeshDensityMockMesh(int rank) {
    YogaMesh mesh;
    mesh.setNodeCount(0 == rank ? 5 : 0);
    mesh.setXyzForNodes([](int id, double* xyz) {
        xyz[0] = .5 + id;
        xyz[1] = xyz[2] = 0.0;
    });
    mesh.setOwningRankForNodes([=](int id) { return rank; });
    return mesh;
}

TEST_CASE("tally nodes in parallel") {
    MessagePasser mp(MPI_COMM_WORLD);

    auto mesh = generateMeshDensityMockMesh(mp.Rank());
    CartBlock block({0, 0, 0}, {5, 5, 5}, 5, 5, 5);
    std::vector<Parfait::Extent<double>> component_extents = {block,block};
    auto nodeCountsPerCell =
        MeshDensityEstimator::tallyNodesContainedByCartCells(mp, mesh, component_extents,block);

    //for (int i = 0; i < 5; ++i) REQUIRE(1 == nodeCountsPerCell[i]);
    //for (int i = 5; i < 25; ++i) REQUIRE(0 == nodeCountsPerCell[i]);
}
