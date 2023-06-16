#include <RingAssertions.h>
#include <t-infinity/StructuredMeshPartitioner.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/SMesh.h>
#include <t-infinity/StructuredMeshHelpers.h>
#include <t-infinity/PartitionedStructuredMesh.h>

using namespace inf;

using Point = Parfait::Point<double>;

SMesh buildSingleMeshBlockOnRoot(MessagePasser mp) {
    inf::SMesh mesh;
    if (mp.Rank() == 0) {
        auto block = std::make_shared<inf::CartesianBlock>(
            Point{0, 0, 0}, Point{1, 1, 1}, std::array<int, 3>{10, 10, 10}, 0);
        mesh.add(block);
    }
    return mesh;
}

TEST_CASE("Can partition a StructuredMesh") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) return;
    auto mesh = buildSingleMeshBlockOnRoot(mp);
    auto global_node_ids = assignUniqueGlobalNodeIds(mp, mesh);
    SECTION("Split block") {
        auto block = StructuredMeshPartitioner::buildBlockMappings(global_node_ids).at(0);
        REQUIRE(0 == block.original_block_id);
        REQUIRE(std::array<int, 3>{11, 11, 11} == block.nodeDimensions());
        auto split_blocks = block.split(inf::J);
        REQUIRE(0 == split_blocks[0].original_block_id);
        REQUIRE(std::array<int, 3>{0, 0, 0} == split_blocks[0].original_node_range_min);
        REQUIRE(std::array<int, 3>{11, 6, 11} == split_blocks[0].original_node_range_max);
        REQUIRE(0 == split_blocks[1].original_block_id);
        REQUIRE(std::array<int, 3>{0, 5, 0} == split_blocks[1].original_node_range_min);
        REQUIRE(std::array<int, 3>{11, 11, 11} == split_blocks[1].original_node_range_max);
        loopMDRange({0, 0, 0}, split_blocks.at(0).nodeDimensions(), [&](int i, int j, int k) {
            INFO("ijk: (" << i << "," << j << "," << k << ")");
            REQUIRE(split_blocks.at(0).isOwnedNode(i, j, k));
        });
        loopMDRange({0, 0, 0}, split_blocks.at(0).nodeDimensions(), [&](int i, int j, int k) {
            INFO("ijk: (" << i << "," << j << "," << k << ")");
            bool node_is_owned = j != 0;
            REQUIRE(node_is_owned == split_blocks.at(1).isOwnedNode(i, j, k));
        });
    }
    SECTION("Split mesh") {
        auto block_mappings = StructuredMeshPartitioner::buildBlockMappings(global_node_ids);
        REQUIRE(1 == block_mappings.size());
        REQUIRE(0 == block_mappings.at(0).original_block_id);
        REQUIRE(std::array<int, 3>{0, 0, 0} == block_mappings.at(0).original_node_range_min);
        REQUIRE(std::array<int, 3>{11, 11, 11} == block_mappings.at(0).original_node_range_max);

        StructuredMeshPartitioner::splitLargestBlock(mp, block_mappings);
        REQUIRE(2 == block_mappings.size());
        REQUIRE(0 == block_mappings.at(0).original_block_id);
        REQUIRE(0 == block_mappings.at(1).original_block_id);
        REQUIRE(std::array<int, 3>{0, 0, 0} == block_mappings.at(0).original_node_range_min);
        REQUIRE(std::array<int, 3>{6, 11, 11} == block_mappings.at(0).original_node_range_max);
        REQUIRE(std::array<int, 3>{5, 0, 0} == block_mappings.at(1).original_node_range_min);
        REQUIRE(std::array<int, 3>{11, 11, 11} == block_mappings.at(1).original_node_range_max);
    }
    SECTION("Split global node ids") {
        auto block_mappings = StructuredMeshPartitioner::buildBlockMappings(global_node_ids);
        StructuredMeshPartitioner::splitLargestBlock(mp, block_mappings);
        auto split_nodes = StructuredMeshPartitioner::splitGlobalNodes(block_mappings, global_node_ids.ids);
        REQUIRE(2 == split_nodes.size());
        for (const auto& p : split_nodes) {
            const auto& block = block_mappings.at(p.first);
            REQUIRE(block.nodeDimensions() == p.second.dimensions());
            auto offset = block.original_node_range_min;
            int original_block_id = block.original_block_id;
            loopMDRange({0, 0, 0}, p.second.dimensions(), [&](int i, int j, int k) {
                auto expected = global_node_ids.ids.at(original_block_id)(i + offset[0], j + offset[1], k + offset[2]);
                REQUIRE(expected == p.second(i, j, k));
            });
        }
    }
    SECTION("Re-index split global ids to be contiguous by block") {
        auto block_mappings = StructuredMeshPartitioner::buildBlockMappings(global_node_ids);
        StructuredMeshPartitioner::splitLargestBlock(mp, block_mappings);
        auto split_nodes = StructuredMeshPartitioner::splitGlobalNodes(block_mappings, global_node_ids.ids);
        StructuredMeshPartitioner::makeGlobalNodeIdsContiguousByBlock(split_nodes, mp, block_mappings);
        long max_gid = 0;
        for (const auto& p : block_mappings) {
            loopMDRange({0, 0, 0}, p.second.nodeDimensions(), [&](int i, int j, int k) {
                max_gid = std::max(max_gid, split_nodes.at(p.first)(i, j, k));
            });
        }
        max_gid = mp.ParallelMax(max_gid);
        REQUIRE(1330 == max_gid);
    }
}

TEST_CASE("Can build a PartitionedStructuredMesh and rebalance it") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto single_block = buildSingleMeshBlockOnRoot(mp);
    SECTION("Original") {
        auto partitioned = PartitionedStructuredMesh(mp, single_block, assignUniqueGlobalNodeIds(mp, single_block));
        REQUIRE(std::pow(11, 3) == partitioned.globalNodeCount());
        REQUIRE(std::pow(10, 3) == partitioned.globalCellCount());
        auto mesh = partitioned.getMesh();
        std::vector<int> expected_blocks_per_rank = {1, 0};
        REQUIRE(expected_blocks_per_rank[mp.Rank()] == (int)mesh->blockIds().size());
        REQUIRE(1.0 / mp.NumberOfProcesses() == Approx(partitioned.getCurrentLoadBalance()));
    }
    SECTION("Partitioned and balanced") {
        auto original_global_node_ids = assignUniqueGlobalNodeIds(mp, single_block);
        auto partitioned = PartitionedStructuredMesh(mp, single_block, original_global_node_ids);
        partitioned.balance();
        auto mesh = partitioned.getMesh();
        REQUIRE(std::pow(11, 3) == partitioned.globalNodeCount());
        REQUIRE(std::pow(10, 3) == partitioned.globalCellCount());
        std::vector<int> expected_blocks_per_rank = {1, 1};
        REQUIRE(expected_blocks_per_rank[mp.Rank()] == (int)mesh->blockIds().size());
        REQUIRE(1.0 == Approx(partitioned.getCurrentLoadBalance()));
    }
}