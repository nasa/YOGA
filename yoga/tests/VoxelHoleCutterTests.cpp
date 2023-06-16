#include "VoxelDonorFinder.h"

using namespace YOGA;
using namespace Parfait;

class MockWorkVoxelHoleMap {
  public:
    bool doesOverlapHole(Extent<double>& e) { return false; }
    int getAssociatedComponentId() { return 0; }
};
#if 0
TEST_CASE("Process work voxel with two overlapping tets, but only one node is actually in the voxel and has donors"){
    Extent<double> voxelExtent {{1,0,1},{3,1,3}};
    WorkVoxel workVoxel(voxelExtent);
    workVoxel.nodes.push_back(TransferNode(0,{0,0,0},0,BoundaryConditions::NotABoundary ,7));
    workVoxel.nodes.push_back(TransferNode(1,{5,0,0},0,BoundaryConditions::NotABoundary ,7));
    workVoxel.nodes.push_back(TransferNode(2,{0,5,0},0,BoundaryConditions::NotABoundary ,7));
    workVoxel.nodes.push_back(TransferNode(3,{0,0,5},0,BoundaryConditions::NotABoundary, 7));
    workVoxel.nodes.push_back(TransferNode(4,{2,0.5,2},1,BoundaryConditions::Interpolation ,0)); // set node as
                                                                        // interpolation
                                                                        // boundary so it gets interpolated
                                                                        // for either implicit or explicit hole cutting
    workVoxel.nodes.push_back(TransferNode(5,{7,2,2},1,BoundaryConditions::NotABoundary ,0));
    workVoxel.nodes.push_back(TransferNode(6,{0,7,0},1,BoundaryConditions::NotABoundary ,0));
    workVoxel.nodes.push_back(TransferNode(7,{0,2,7},1,BoundaryConditions::NotABoundary ,0));
    workVoxel.tets.push_back({0,1,2,3});
    workVoxel.tets.push_back({4,5,6,7});

    workVoxel.visualize("work voxel");

    MockWorkVoxelHoleMap holeMap;
    std::vector<decltype(holeMap)> holeMaps {holeMap};

    VoxelHoleCutter holeCutter;
    auto connectivityInfo = holeCutter.explicitHoleCut(workVoxel,voxelExtent,holeMaps);
    REQUIRE(connectivityInfo.nodeStatuses.size() == workVoxel.nodes.size());

    REQUIRE(VoxelHoleCutter::NotInVoxel == connectivityInfo.nodeStatuses[0]);
    REQUIRE(VoxelHoleCutter::NotInVoxel == connectivityInfo.nodeStatuses[1]);
    REQUIRE(VoxelHoleCutter::NotInVoxel == connectivityInfo.nodeStatuses[2]);
    REQUIRE(VoxelHoleCutter::NotInVoxel == connectivityInfo.nodeStatuses[3]);
    REQUIRE(VoxelHoleCutter::Receptor == connectivityInfo.nodeStatuses[4]);
    REQUIRE(VoxelHoleCutter::NotInVoxel == connectivityInfo.nodeStatuses[5]);
    REQUIRE(VoxelHoleCutter::NotInVoxel == connectivityInfo.nodeStatuses[6]);
    REQUIRE(VoxelHoleCutter::NotInVoxel == connectivityInfo.nodeStatuses[7]);

    REQUIRE(connectivityInfo.donorIds[4].size() == 4);
    REQUIRE(connectivityInfo.donorIds[4][0] == 0);
    REQUIRE(connectivityInfo.donorIds[4][1] == 1);
    REQUIRE(connectivityInfo.donorIds[4][2] == 2);
    REQUIRE(connectivityInfo.donorIds[4][3] == 3);

    REQUIRE(connectivityInfo.donorWeights[4].size() == 4);

    // make sure interpolation weights can correctly reconstruct the xyz value of the query point
    Point<double> interpolatedValue {0,0,0};
    for(int i=0;i<4;++i)
        interpolatedValue += connectivityInfo.donorWeights[4][i] * workVoxel.nodes[connectivityInfo.donorIds[4][i]].xyz;
    REQUIRE(interpolatedValue.approxEqual(workVoxel.nodes[4].xyz,0.00001));

    REQUIRE(connectivityInfo.donorOwners[4].size() == 4);
    for(int i=0;i<4;++i)
        REQUIRE(connectivityInfo.donorOwners[4][i] == 7);
}

#endif