#include <VoxelFragment.h>
#include "WorkVoxel.h"
#include <RingAssertions.h>
#include <parfait/CellContainmentChecker.h>

using namespace std;
using namespace YOGA;

TEST_CASE("Work Voxel test") {
    Parfait::Extent<double> e({0, 0, 0}, {1, 1, 1});
    WorkVoxel workVoxel(e,&Parfait::CellContainmentChecker::isInCell_c);
    vector<TransferNode> nodes;
    nodes.push_back(TransferNode(8, {0, 0, 0}, 0.0, 0, 0));
    nodes.push_back(TransferNode(1, {1, 0, 0}, 0.0, 0, 0));
    nodes.push_back(TransferNode(3, {0, 1, 0}, 0.0, 0, 0));
    nodes.push_back(TransferNode(6, {0, 0, 1}, 0.0, 0, 0));
    vector<TransferCell<4>> tets;
    tets.push_back(TransferCell<4>({0, 1, 2, 3}, 0, 0));

    std::vector<int> new_local_ids;
    workVoxel.addNodes(nodes,new_local_ids);
    workVoxel.addTets(tets, nodes, new_local_ids);

    REQUIRE(4 == workVoxel.nodes.size());
    REQUIRE(1 == workVoxel.tets.size());
    // make sure that tet is in local numbering
    REQUIRE(0 == workVoxel.tets[0].nodeIds[0]);
    REQUIRE(1 == workVoxel.tets[0].nodeIds[1]);
    REQUIRE(2 == workVoxel.tets[0].nodeIds[2]);
    REQUIRE(3 == workVoxel.tets[0].nodeIds[3]);

    vector<TransferNode> moreNodes;
    moreNodes.push_back(TransferNode(10, {0, 0, 1}, 0.0, 0, 0));
    moreNodes.push_back(TransferNode(11, {1, 0, 1}, 0.0, 0, 0));
    moreNodes.push_back(TransferNode(12, {0, 1, 1}, 0.0, 0, 0));
    moreNodes.push_back(TransferNode(13, {0, 0, 2}, 0.0, 0, 0));
    vector<TransferCell<4>> anotherTet;
    anotherTet.push_back(TransferCell<4>({0, 1, 2, 3}, 1, 0));

    workVoxel.addNodes(moreNodes,new_local_ids);
    workVoxel.addTets(anotherTet, moreNodes,new_local_ids);

    REQUIRE(8 == workVoxel.nodes.size());
    REQUIRE(2 == workVoxel.tets.size());
    REQUIRE(4 == workVoxel.tets[1].nodeIds[0]);
    REQUIRE(5 == workVoxel.tets[1].nodeIds[1]);
    REQUIRE(6 == workVoxel.tets[1].nodeIds[2]);
    REQUIRE(7 == workVoxel.tets[1].nodeIds[3]);
}
