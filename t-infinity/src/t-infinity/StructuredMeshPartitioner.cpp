#include "StructuredMeshPartitioner.h"
#include "StructuredMeshHelpers.h"
inf::StructuredMeshPartitioner::BlockMappings inf::StructuredMeshPartitioner::buildBlockMappings(
    const inf::StructuredMeshGlobalIds& global_node) {
    BlockMappings blocks;
    for (const auto& p : global_node.ids) {
        int block_id = p.first;
        blocks[block_id] = SplitStructuredBlockMapping(block_id, global_node);
    }
    return blocks;
}
template <typename Shift>
inf::StructuredBlockGlobalIds splitGlobalIds(
    const inf::StructuredMeshPartitioner::BlockMappings& blocks,
    const inf::StructuredBlockGlobalIds& global_ids,
    Shift shift_dimensions) {
    inf::StructuredBlockGlobalIds split_global_ids;
    for (const auto& p : global_ids) {
        int original_block_id = p.first;
        const auto& original_ids = p.second;

        for (const auto& split : blocks) {
            if (split.second.original_block_id == original_block_id) {
                auto dimensions = shift_dimensions(split.second.nodeDimensions());
                inf::Vector3D<long> split_ids(dimensions);
                auto start = split.second.original_node_range_min;
                auto end = shift_dimensions(split.second.original_node_range_max);
                inf::loopMDRange(start, end, [&](int i, int j, int k) {
                    split_ids(i - start[0], j - start[1], k - start[2]) = original_ids(i, j, k);
                });
                split_global_ids[split.first] = split_ids;
            }
        }
    }

    return split_global_ids;
}
inf::StructuredBlockGlobalIds inf::StructuredMeshPartitioner::splitGlobalNodes(
    const inf::StructuredMeshPartitioner::BlockMappings& blocks,
    const inf::StructuredBlockGlobalIds& global_node_ids) {
    auto no_shift = [](const auto& v) { return v; };
    return splitGlobalIds(blocks, global_node_ids, no_shift);
}
inf::StructuredBlockGlobalIds inf::StructuredMeshPartitioner::splitGlobalCells(
    const inf::StructuredMeshPartitioner::BlockMappings& blocks,
    const inf::StructuredBlockGlobalIds& global_cell) {
    auto shift_cell_dimensions = [](auto v) {
        for (auto& d : v) d = std::max(d - 1, 0);
        return v;
    };
    return splitGlobalIds(blocks, global_cell, shift_cell_dimensions);
}
void inf::StructuredMeshPartitioner::splitLargestBlock(
    const MessagePasser& mp, inf::StructuredMeshPartitioner::BlockMappings& blocks) {
    int largest_block_node_count = 0;
    if (not blocks.empty()) {
        auto largest_block_id = findBlockWithLargestNumberOfNodes(blocks);
        largest_block_node_count = blocks.at(largest_block_id).nodeCount();
    }
    int rank_with_largest_block = mp.ParallelRankOfMax(largest_block_node_count);
    int next_available_block_id = getNextAvailableBlockId(mp, blocks);
    if (mp.Rank() == rank_with_largest_block) {
        auto largest_block_id = findBlockWithLargestNumberOfNodes(blocks);
        auto& largest_block = blocks.at(largest_block_id);
        auto axis_to_split = findLargestAxis(largest_block.nodeDimensions());
        auto split_blocks = largest_block.split(axis_to_split);
        blocks[largest_block_id] = split_blocks.front();
        blocks[next_available_block_id] = split_blocks.back();
    }
}
int inf::StructuredMeshPartitioner::getNextAvailableBlockId(
    const MessagePasser& mp, const inf::StructuredMeshPartitioner::BlockMappings& blocks) {
    int largest_block_id = 0;
    for (const auto& b : blocks) largest_block_id = std::max(b.first, largest_block_id);
    return mp.ParallelMax(largest_block_id) + 1;
}
int inf::StructuredMeshPartitioner::findBlockWithLargestNumberOfNodes(
    const inf::StructuredMeshPartitioner::BlockMappings& blocks) {
    int block_id = blocks.begin()->first;
    int largest_node_count = 0;
    for (const auto& b : blocks) {
        int node_count = b.second.nodeCount();
        if (node_count > largest_node_count) {
            block_id = b.first;
            largest_node_count = node_count;
        }
    }
    return block_id;
}
inf::BlockAxis inf::StructuredMeshPartitioner::findLargestAxis(
    const std::array<int, 3>& node_dimensions) {
    BlockAxis largest_axis = inf::I;
    int largest_axis_node_count = 0;
    for (auto axis : {I, J, K}) {
        if (node_dimensions[axis] > largest_axis_node_count) {
            largest_axis = axis;
            largest_axis_node_count = node_dimensions[axis];
        }
    }
    return largest_axis;
}

std::map<int, int> inf::StructuredMeshPartitioner::buildBlockOwnedCounts(
    const MessagePasser& mp, const inf::StructuredMeshPartitioner::BlockMappings& blocks) {
    std::vector<int> block_ids, owned_counts;
    for (const auto& p : blocks) {
        block_ids.push_back(p.first);
        owned_counts.push_back(p.second.ownedCount());
    }
    auto all_block_ids = mp.Gather(block_ids, 0);
    auto all_owned_counts = mp.Gather(owned_counts, 0);
    std::map<int, int> block_owned_counts;
    if (mp.Rank() == 0) {
        for (int rank = 0; rank < mp.NumberOfProcesses(); ++rank) {
            auto n_blocks = all_block_ids.at(rank).size();
            PARFAIT_ASSERT(n_blocks == all_owned_counts.at(rank).size(),
                           "Mismatch in block counts");
            for (size_t i = 0; i < n_blocks; ++i) {
                int block_id = all_block_ids.at(rank).at(i);
                int block_owned_node_count = all_owned_counts.at(rank).at(i);
                block_owned_counts[block_id] = block_owned_node_count;
            }
        }
    }
    mp.Broadcast(block_owned_counts, 0);
    return block_owned_counts;
}
inf::StructuredMeshGlobalIdOwner inf::StructuredMeshPartitioner::buildSplitBlockOwnership(
    const MessagePasser& mp, const inf::StructuredMeshPartitioner::BlockMappings& blocks) {
    auto block_owned_counts = buildBlockOwnedCounts(mp, blocks);
    std::map<int, std::array<long, 2>> ownership_ranges;
    long count = 0;
    for (const auto& p : block_owned_counts) {
        ownership_ranges[p.first].front() = count;
        count += p.second;
        ownership_ranges[p.first].back() = count - 1;
    }
    std::vector<int> blocks_on_rank;
    for (const auto& p : blocks) blocks_on_rank.push_back(p.first);
    auto block_to_rank = buildBlockToRank(mp, blocks_on_rank);
    return {ownership_ranges, block_to_rank};
}

inf::StructuredBlockGlobalIds makeOwnedGlobalNodeIdsContiguousByBlock(
    inf::StructuredBlockGlobalIds global_node_ids,
    const MessagePasser& mp,
    const inf::StructuredMeshPartitioner::BlockMappings& blocks) {
    using namespace inf;
    auto block_owned_counts = StructuredMeshPartitioner::buildBlockOwnedCounts(mp, blocks);
    for (const auto& p : blocks) {
        int block_id = p.first;
        auto& ids = global_node_ids[block_id];
        long gid = 0;
        for (const auto& b : block_owned_counts)
            if (b.first < block_id) gid += b.second;
        loopMDRange({0, 0, 0}, ids.dimensions(), [&](int i, int j, int k) {
            if (p.second.isOwnedNode(i, j, k)) ids(i, j, k) = gid++;
        });
    }
    return global_node_ids;
}
inf::StructuredBlockGlobalIds inf::StructuredMeshPartitioner::makeGlobalNodeIdsContiguousByBlock(
    const StructuredBlockGlobalIds& global_ids,
    const MessagePasser& mp,
    const inf::StructuredMeshPartitioner::BlockMappings& blocks) {
    auto contiguous_global_ids = makeOwnedGlobalNodeIdsContiguousByBlock(global_ids, mp, blocks);

    // 1. Build global-to-global
    std::map<long, long> old_to_new_global_ids;
    for (const auto& p : blocks) {
        loopMDRange({0, 0, 0}, p.second.nodeDimensions(), [&](int i, int j, int k) {
            if (p.second.owned_by_block(i, j, k)) {
                long original_gid = global_ids.at(p.first)(i, j, k);
                long new_gid = contiguous_global_ids.at(p.first)(i, j, k);
                old_to_new_global_ids[original_gid] = new_gid;
            }
        });
    }

    // 2. accumulate and distribute global ids
    std::map<int, MessagePasser::Message> messages;
    auto& msg = messages[0];
    msg.pack(old_to_new_global_ids);
    messages = mp.Exchange(messages);
    if (mp.Rank() == 0) {
        for (auto& m : messages) {
            std::map<long, long> g2g;
            m.second.unpack(g2g);
            for (const auto& p : g2g) old_to_new_global_ids[p.first] = p.second;
        }
    }
    mp.Broadcast(old_to_new_global_ids, 0);

    // 3. Set the rest of the global ids
    for (auto& p : contiguous_global_ids) {
        loopMDRange({0, 0, 0}, p.second.dimensions(), [&](int i, int j, int k) {
            if (not blocks.at(p.first).isOwnedNode(i, j, k)) {
                long& gid = p.second(i, j, k);
                gid = old_to_new_global_ids.at(gid);
            }
        });
    }
    return contiguous_global_ids;
}

inf::StructuredMeshGlobalIds inf::StructuredMeshPartitioner::buildNewGlobalNodes(
    const MessagePasser& mp,
    const inf::StructuredMeshPartitioner::BlockMappings& blocks,
    const inf::StructuredMeshGlobalIds& global_node) {
    auto owners = buildSplitBlockOwnership(mp, blocks);
    auto split_global_ids = splitGlobalNodes(blocks, global_node.ids);
    return {makeGlobalNodeIdsContiguousByBlock(split_global_ids, mp, blocks), owners};
}
