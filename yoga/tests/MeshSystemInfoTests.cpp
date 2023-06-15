#include <MessagePasser/MessagePasser.h>
#include <parfait/Point.h>
#include <RingAssertions.h>
#include "DiagonalTetsMockMesh.h"
#include "MeshSystemInfo.h"
#include "PartitionInfo.h"

using namespace Parfait;
using namespace YOGA;

TEST_CASE("Make sure each rank gets global info correct") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto mesh = generateDiagonalTetsMockMesh(mp.Rank());
    YOGA::PartitionInfo partition_info(mesh, mp.Rank());
    YOGA::MeshSystemInfo meshSystemInfo(mp, partition_info);

    REQUIRE(1 == meshSystemInfo.numberOfBodies());
    REQUIRE(1 == meshSystemInfo.numberOfComponents());

    REQUIRE(0 == meshSystemInfo.getComponentIdForBody(0));

    SECTION("There is only '1' body, which consists of unit cubes along the diagonal") {
        auto body_extent = meshSystemInfo.getBodyExtent(0);
        REQUIRE(0 == Approx(body_extent.lo[0]));
        REQUIRE(0 == Approx(body_extent.lo[1]));
        REQUIRE(0 == Approx(body_extent.lo[2]));
        REQUIRE((mp.NumberOfProcesses()) == Approx(body_extent.hi[0]));
        REQUIRE((mp.NumberOfProcesses()) == Approx(body_extent.hi[1]));
        REQUIRE((mp.NumberOfProcesses()) == Approx(body_extent.hi[2]));
    }

    SECTION("There is only '1' component mesh, so check that all ranks have the same extent") {
        auto component_extent = meshSystemInfo.getComponentExtent(0);
        REQUIRE(0 == component_extent.lo[0]);
        REQUIRE(0 == component_extent.lo[1]);
        REQUIRE(0 == component_extent.lo[2]);
        REQUIRE((mp.NumberOfProcesses()) == component_extent.hi[0]);
        REQUIRE((mp.NumberOfProcesses()) == component_extent.hi[1]);
        REQUIRE((mp.NumberOfProcesses()) == component_extent.hi[2]);
    }

    REQUIRE(mp.NumberOfProcesses() == meshSystemInfo.numberOfPartitions());
}
