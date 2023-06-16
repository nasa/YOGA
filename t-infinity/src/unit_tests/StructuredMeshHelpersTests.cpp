#include <RingAssertions.h>
#include <t-infinity/StructuredMeshHelpers.h>
#include <t-infinity/SMesh.h>

using namespace inf;

TEST_CASE("Can build structured mesh block-to-rank") {
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<int> blocks_on_rank;
    if (mp.Rank() == 0) {
        blocks_on_rank.push_back(0);
        blocks_on_rank.push_back(2);
    }
    if (mp.Rank() == mp.NumberOfProcesses() - 1) {
        blocks_on_rank.push_back(10);
        blocks_on_rank.push_back(201);
    }
    auto block_to_rank = buildBlockToRank(mp, blocks_on_rank);
    REQUIRE(4 == block_to_rank.size());
    REQUIRE(0 == block_to_rank.at(0));
    REQUIRE(0 == block_to_rank.at(2));
    REQUIRE(mp.NumberOfProcesses() - 1 == block_to_rank.at(10));
    REQUIRE(mp.NumberOfProcesses() - 1 == block_to_rank.at(201));
}

TEST_CASE("Can detect a 2D StructuredMesh") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    SMesh mesh;
    mesh.add(SMesh::createBlock({1, 5, 5}, 0));
    mesh.add(SMesh::createBlock({1, 2, 2}, 1));
    REQUIRE(is2D(mp, mesh));
    if (mp.Rank() == mp.NumberOfProcesses() - 1) mesh.add(SMesh::createBlock({3, 2, 2}, 2));
    //    mesh.add(SMesh::createBlock({3, 2, 2}, 2));
    REQUIRE_FALSE(is2D(mp, mesh));
}

TEST_CASE("Can build map of all StructuredBlock dimensions on all ranks") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    SMesh mesh;
    if (mp.Rank() == 0) mesh.add(SMesh::createBlock({1, 5, 5}, 0));
    if (mp.Rank() == mp.NumberOfProcesses() - 1) mesh.add(SMesh::createBlock({1, 2, 2}, 1));
    auto all_dimensions = buildAllBlockDimensions(mp, mesh);
    REQUIRE(2 == all_dimensions.size());
    REQUIRE(std::array<int, 3>{1,5,5} == all_dimensions.at(0));
    REQUIRE(std::array<int, 3>{1,2,2} == all_dimensions.at(1));
}