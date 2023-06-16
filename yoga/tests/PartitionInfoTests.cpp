#include <MessagePasser/MessagePasser.h>
#include <parfait/Point.h>
#include <RingAssertions.h>
#include "PartitionInfo.h"

using namespace Parfait;
using namespace YOGA;

auto generateMockMesh() {
    YogaMesh mesh;
    mesh.setFaceCount(1);
    mesh.setFaces([](int) { return 3; },
                  [](int, int* face) {
                      face[0] = 0;
                      face[1] = 1;
                      face[2] = 2;
                  });
    mesh.setBoundaryConditions([](int) { return BoundaryConditions ::Solid; }, [](int){return 3;});
    mesh.setNodeCount(3);
    mesh.setXyzForNodes([](int id, double* xyz) { xyz[0] = xyz[1] = xyz[2] = id; });
    mesh.setComponentIdsForNodes([](int) { return 0; });
    mesh.setCellCount(0);
    return mesh;
}

TEST_CASE("partition info stuff") {
    auto mesh = generateMockMesh();

    std::vector<double> distanceToWall(mesh.nodeCount(), 0.0);
    PartitionInfo partitionInfo(mesh, 0);
    REQUIRE(1 == partitionInfo.numberOfBodies());
    auto body_extent = partitionInfo.getExtentForBody(0);
    REQUIRE(0.0 == Approx(body_extent.lo[0]));
    REQUIRE(0.0 == Approx(body_extent.lo[1]));
    REQUIRE(0.0 == Approx(body_extent.lo[2]));
    REQUIRE(2.0 == Approx(body_extent.hi[0]));
    REQUIRE(2.0 == Approx(body_extent.hi[1]));
    REQUIRE(2.0 == Approx(body_extent.hi[2]));
    REQUIRE(1 == partitionInfo.numberOfComponentMeshes());

    auto partition_extent = partitionInfo.getExtentForComponent(0);
    REQUIRE(0.0 == partition_extent.lo[0]);
    REQUIRE(0.0 == partition_extent.lo[1]);
    REQUIRE(0.0 == partition_extent.lo[2]);
    REQUIRE(2.0 == partition_extent.hi[0]);
    REQUIRE(2.0 == partition_extent.hi[1]);
    REQUIRE(2.0 == partition_extent.hi[2]);

    REQUIRE_THROWS(partitionInfo.getExtentForComponent(1));
    REQUIRE_THROWS(partitionInfo.getExtentForBody(1));

    REQUIRE(partitionInfo.containsBody(0));
    REQUIRE_FALSE(partitionInfo.containsBody(1));

    REQUIRE(partitionInfo.containsComponentMesh(0));
    REQUIRE_FALSE(partitionInfo.containsComponentMesh(1));

    auto body_ids = partitionInfo.getBodyIds();
    REQUIRE(1 == body_ids.size());
    REQUIRE(0 == body_ids.front());

    auto component_ids = partitionInfo.getComponentIds();
    REQUIRE(1 == component_ids.size());
    REQUIRE(0 == component_ids.front());
}