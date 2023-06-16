#include <MeshSystemInfo.h>
#include <RingAssertions.h>
#include "OverlapMask.h"

using namespace YOGA;

class MockSystemInfo : public MeshSystemInfo {
  public:
    MockSystemInfo(MessagePasser mp) : MeshSystemInfo(mp) {
        component_extents.push_back(Parfait::Extent<double>({0, 0, 0}, {2, 2, 2}));
        component_extents.push_back(Parfait::Extent<double>({1, 1, 1}, {3, 3, 3}));
    }

  private:
};

auto generateOverlapMaskMockMesh() {
    YogaMesh mesh;
    mesh.setNodeCount(2);
    mesh.setXyzForNodes([](int id, double* xyz) { xyz[0] = xyz[1] = xyz[2] = id; });
    return mesh;
}

TEST_CASE("build a mask that marks nodes that are potentially in overlap regions") {
    auto mesh = generateOverlapMaskMockMesh();
    MessagePasser mp(MPI_COMM_WORLD);
    MockSystemInfo meshSystemInfo(mp);
    std::vector<bool> mask = YOGA::OverlapMask::buildNodeMask(mesh, meshSystemInfo);

    REQUIRE(2 == mask.size());
    REQUIRE_FALSE(mask[0]);
    REQUIRE(mask[1]);
}
