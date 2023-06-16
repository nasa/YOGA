#include <RingAssertions.h>
#include "DiagonalTetsMockMesh.h"
#include "ScalableHoleMap.h"

using namespace Parfait;
using namespace YOGA;

TEST_CASE("Make sure test mesh has what I expect on every proc") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto mesh = generateDiagonalTetsMockMesh(mp.Rank());
    PartitionInfo partition_info(mesh, mp.Rank());
    MeshSystemInfo systemInfo(mp, partition_info);
    int n = mp.NumberOfProcesses();
    REQUIRE(1 == systemInfo.numberOfBodies());
    auto e = systemInfo.getBodyExtent(0);

    REQUIRE(0 == Approx(e.lo[0]));
    REQUIRE(0 == Approx(e.lo[1]));
    REQUIRE(0 == Approx(e.lo[2]));
    REQUIRE(n == Approx(e.hi[0]));
    REQUIRE(n == Approx(e.hi[1]));
    REQUIRE(n == Approx(e.hi[2]));
}

TEST_CASE("Check blanking...") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto mesh = generateDiagonalTetsMockMesh(mp.Rank());
    std::vector<double> distanceToWall(mesh.nodeCount(), 0.0);
    PartitionInfo partition_info(mesh, mp.Rank());
    MeshSystemInfo meshSystemInfo(mp, partition_info);
    ScalableHoleMap hole_map(mp, mesh, partition_info, meshSystemInfo, 0, 5);
    SECTION("Make sure each proc blanks it's local stuff") {
        Extent<double> search_extent{{0., 0., 0.}, {.5, .5, .5}};
        Point<double> proc_offset{double(mp.Rank()), double(mp.Rank()), double(mp.Rank())};
        search_extent.lo += proc_offset;
        search_extent.hi += proc_offset;
        REQUIRE(hole_map.doesOverlapHole(search_extent));
    }

    SECTION("Furthermore, check that they see holes from non-local geometry also") {
        for (int i = 0; i < mp.NumberOfProcesses(); i++) {
            Extent<double> search_extent{{0., 0., 0.}, {.5, .5, .5}};
            Point<double> proc_offset{double(i), double(i), double(i)};
            search_extent.lo += proc_offset;
            search_extent.hi += proc_offset;
            REQUIRE(hole_map.doesOverlapHole(search_extent));
        }
    }
}
