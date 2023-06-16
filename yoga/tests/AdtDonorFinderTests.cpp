#include <RingAssertions.h>
#include <vector>
#include "AdtDonorFinder.h"
#include <parfait/CellContainmentChecker.h>

using namespace YOGA;
using namespace std;
TEST_CASE("build adt") {
    WorkVoxel workVoxel(Parfait::Extent<double>({0, 0, 0}, {1, 1, 1}),&Parfait::CellContainmentChecker::isInCell_c);
    vector<TransferNode> nodes;

    SECTION("test with tet") {
        nodes.push_back(TransferNode(8, {0, 0, 0}, 0.0, 0, 0));
        nodes.push_back(TransferNode(1, {1, 0, 0}, 0.0, 0, 0));
        nodes.push_back(TransferNode(3, {0, 1, 0}, 0.0, 0, 0));
        nodes.push_back(TransferNode(6, {0, 0, 1}, 0.0, 0, 0));
        std::vector<int> new_local_ids;
        workVoxel.addNodes(nodes,new_local_ids);

        vector<TransferCell<4>> tets;
        int cell_id = 99;
        int owning_rank = 0;
        tets.push_back(TransferCell<4>({0, 1, 2, 3}, cell_id, owning_rank));
        workVoxel.addTets(tets, nodes, new_local_ids);

        AdtDonorFinder adtDonorFinder(workVoxel);
        SECTION("find donor for point in some other component mesh than the tet") {
            int component_of_query_point = 3;
            Parfait::Point<double> p {.1,.1,.1};
            auto candidates = adtDonorFinder.findDonors(p, component_of_query_point);

            REQUIRE(1 == candidates.size());
            REQUIRE(cell_id == candidates.front().cellId);
            REQUIRE(owning_rank == candidates.front().cellOwner);
        }
        SECTION("don't return candidates from the same component as the query point") {
            int component_of_query_point = owning_rank;
            Parfait::Point<double> p {.1,.1,.1};
            auto candidates = adtDonorFinder.findDonors(p, component_of_query_point);

            REQUIRE(0 == candidates.size());
        }
    }

#if 0
    SECTION("test with pyramid") {
        nodes.push_back(TransferNode(8,  {0, 0, 0}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(1,  {1, 0, 0}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(3,  {1, 1, 0}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(6,  {0, 1, 0}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(10, {0, 0, 1}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        workVoxel.addNodes(nodes);

        vector<TransferCell<5>> pyramids;
        int cell_id = 23;
        int node_owning_rank = 0;
        pyramids.push_back(TransferCell<5>({8, 1, 3, 6, 10}, cell_id, node_owning_rank));
        workVoxel.addPyramids(pyramids);

        AdtDonorFinder adtDonorFinder(workVoxel);
        int component_of_query_point = 3;
        auto candidates = adtDonorFinder.findDonors({0.1,0.1,0.1},component_of_query_point);

        REQUIRE(1 == candidates.size());
        REQUIRE(cell_id == candidates.front().cellId);
        REQUIRE(node_owning_rank == candidates.front().cellOwner);
        REQUIRE(0 == candidates.front().component);
    }

    SECTION("test with prism") {
        nodes.push_back(TransferNode(8,  {0, 0, 0}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(1,  {1, 0, 0}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(6,  {0, 1, 0}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(10,  {0, 0, 1}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(11,  {1, 0, 1}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(12,  {0, 1, 1}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        workVoxel.addNodes(nodes);

        vector<TransferCell<6>> prisms;
        int cell_id = 11;
        int node_owning_rank = 49;
        prisms.push_back(TransferCell<6>({8, 1, 6, 10, 11, 12}, cell_id, node_owning_rank));
        workVoxel.addPrisms(prisms);

        AdtDonorFinder adtDonorFinder(workVoxel);
        int component_of_query_point = 3;
        auto candidates = adtDonorFinder.findDonors({0.1,0.1,0.1},component_of_query_point);

        REQUIRE(1 == candidates.size());
        REQUIRE(cell_id == candidates.front().cellId);
        REQUIRE(node_owning_rank == candidates.front().cellOwner);
        REQUIRE(0 == candidates.front().component);
    }

    SECTION("test with hex") {
        nodes.push_back(TransferNode(8,  {0, 0, 0}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(1,  {1, 0, 0}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(3,  {1, 1, 0}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(6,  {0, 1, 0}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(10,  {0, 0, 1}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(11,  {1, 0, 1}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(12,  {1, 1, 1}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        nodes.push_back(TransferNode(13,  {0, 1, 1}, 0.0, 0, BoundaryConditions::NotABoundary, 0));
        workVoxel.addNodes(nodes);

        vector<TransferCell<8>> hexs;
        int cell_id = 98;
        int node_owning_rank = 4;
        hexs.push_back(TransferCell<8>({8, 1, 3, 6, 10, 11, 12, 13}, cell_id, node_owning_rank));
        workVoxel.addHexs(hexs);

        AdtDonorFinder adtDonorFinder(workVoxel);
        int component_of_query_point = 3;
        auto candidates = adtDonorFinder.findDonors({0.1,0.1,0.1},component_of_query_point);

        REQUIRE(1 == candidates.size());
        REQUIRE(cell_id == candidates.front().cellId);
        REQUIRE(node_owning_rank == candidates.front().cellOwner);
        REQUIRE(0 == candidates.front().component);
    }
#endif
}
