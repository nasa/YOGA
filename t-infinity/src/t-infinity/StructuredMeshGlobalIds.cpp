#include "StructuredMeshGlobalIds.h"
#include "StructuredMeshHelpers.h"
#include "StructuredBlockPlaneSyncer.h"
#include "StructuredMeshShuffle.h"
#include "SMesh.h"
#include <parfait/ExtentBuilder.h>
#include <parfait/UniquePoints.h>
#include <Tracer.h>

using namespace inf;

StructuredBlockGlobalIds initializeGlobalNodeIdsWithBlockId(
    const StructuredBlockDimensions& block_node_dimensions) {
    StructuredBlockGlobalIds global_ids;
    for (const auto& p : block_node_dimensions) {
        int block_id = p.first;
        auto& v = global_ids[block_id].vec;
        global_ids[block_id] = Vector3D<long>(p.second);
        std::fill(v.begin(), v.end(), -block_id - 1);
    }
    return global_ids;
}

void syncGlobalNodeBlockIds(StructuredBlockGlobalIds& global_node_ids,
                            const MessagePasser& mp,
                            const StructuredBlockPlaneSyncer& syncer) {
    TRACER_PROFILE_FUNCTION
    long number_of_node_ids_set = 0;
    // sync planes of global node ids until nothing is updated
    bool keep_syncing = true;
    while (keep_syncing) {
        // syncPlanes is designed to exchange one plane of data for the neighbors data, however here
        // we only want to accept the neighbors data if the global id is "owned" by the block
        // (positive).
        auto getId = [&](int block_id, int i, int j, int k) -> long {
            return global_node_ids[block_id](i, j, k);
        };
        auto setId = [&](int block_id, int i, int j, int k, long gid) -> void {
            int sender_block_id = -gid - 1;
            int receiver_block_id = -global_node_ids[block_id](i, j, k) - 1;
            if (sender_block_id >= receiver_block_id) return;
            global_node_ids[block_id](i, j, k) = gid;
            number_of_node_ids_set += 1;
        };

        syncer.sync(getId, setId, 0);
        keep_syncing = mp.ParallelSum(number_of_node_ids_set) > 0;
        number_of_node_ids_set = 0;
    }
}

std::map<int, int> getBlockOwnedNodeCounts(const MessagePasser& mp,
                                           const StructuredBlockGlobalIds& global_node_ids) {
    std::vector<int> counts;
    for (const auto& p : global_node_ids) {
        int block_id = p.first;
        const auto& global_ids = p.second;
        int count = 0;
        loopMDRange({0, 0, 0}, global_ids.dimensions(), [&](int i, int j, int k) {
            int id = -global_ids(i, j, k) - 1;
            if (block_id == id) count++;
        });
        counts.push_back(count);
    }
    auto all_counts = mp.Gather(counts);
    auto all_blocks = mp.Gather(Parfait::MapTools::keys(global_node_ids));
    std::map<int, int> block_owned_node_count;
    for (int rank = 0; rank < mp.NumberOfProcesses(); ++rank) {
        int l = 0;
        for (int block_id : all_blocks.at(rank)) {
            block_owned_node_count[block_id] = all_counts.at(rank).at(l++);
        }
    }
    return block_owned_node_count;
}

void assignGlobalNodeIds(StructuredBlockGlobalIds& global_node_ids,
                         const std::map<int, int>& owned_node_counts) {
    for (auto& p : global_node_ids) {
        int block_id = p.first;
        auto& global_ids = p.second;

        long offset = 0;
        for (const auto& count : owned_node_counts)
            if (count.first < block_id) offset += count.second;

        loopMDRange({0, 0, 0}, global_ids.dimensions(), [&](int i, int j, int k) {
            int id = -global_ids(i, j, k) - 1;
            if (block_id == id) global_ids(i, j, k) = offset++;
        });
    }
}

StructuredMeshGlobalIds inf::assignStructuredMeshGlobalNodeIds(
    const MessagePasser& mp,
    const MeshConnectivity& mesh_connectivity,
    const StructuredMesh& mesh) {
    StructuredBlockDimensions node_dimensions;
    for (int block_id : mesh.blockIds())
        node_dimensions[block_id] = mesh.getBlock(block_id).nodeDimensions();
    return assignStructuredMeshGlobalNodeIds(mp, mesh_connectivity, node_dimensions);
}

StructuredMeshGlobalIds inf::assignStructuredMeshGlobalNodeIds(
    const MessagePasser& mp,
    const MeshConnectivity& mesh_connectivity,
    const StructuredBlockDimensions& node_dimensions) {
    TRACER_PROFILE_FUNCTION
    for (const auto& p : node_dimensions) {
        PARFAIT_ASSERT(mesh_connectivity.count(p.first) != 0,
                       "Missing connectivity for block: " + std::to_string(p.first));
    }

    // Step 1: Initialize all node ids to block id
    auto global_node_ids = initializeGlobalNodeIdsWithBlockId(node_dimensions);

    std::vector<int> block_ids;
    for (const auto& p : node_dimensions) block_ids.push_back(p.first);
    auto block_to_rank = buildBlockToRank(mp, block_ids);
    auto syncer = StructuredBlockPlaneSyncer(
        mp, mesh_connectivity, node_dimensions, 0, StructuredBlockPlaneSyncer::Node);

    // Step 2: sync all blocks until they agree on the block owner of every node
    syncGlobalNodeBlockIds(global_node_ids, mp, syncer);

    // Step 3: count up all the owned nodes on each block, then build block ownership ranges
    auto owned_nodes_per_block = getBlockOwnedNodeCounts(mp, global_node_ids);
    std::map<int, std::array<long, 2>> block_node_owner_ranges;
    long count = 0;
    for (size_t block_id = 0; block_id < owned_nodes_per_block.size(); ++block_id) {
        block_node_owner_ranges[block_id].front() = count;
        count += owned_nodes_per_block[block_id];
        block_node_owner_ranges[block_id].back() = count - 1;
    }
    StructuredMeshGlobalIdOwner node_owner(block_node_owner_ranges, block_to_rank);

    // Step 4: Now assign the actual global node ids
    assignGlobalNodeIds(global_node_ids, owned_nodes_per_block);

    long number_of_node_ids_set = 0;

    // Step 5: sync actual global ids until everyone is happy
    bool need_sync = true;
    while (need_sync) {
        auto getId = [&](int block_id, int i, int j, int k) -> long {
            return global_node_ids[block_id](i, j, k);
        };
        auto setId = [&](int block_id, int i, int j, int k, long gid) -> void {
            if (gid < 0) return;  // sender did not own the global node id
            auto& current_gid = global_node_ids[block_id](i, j, k);
            if (current_gid >= 0) {
                if (current_gid != gid) {
                    printf("ERROR: global id mismatch\n");
                    printf("block: %d global id -> current: %ld new: %ld\n",
                           block_id,
                           current_gid,
                           gid);
                    printf("global node id: %ld is owned by block: %d\n",
                           current_gid,
                           node_owner.getOwningBlock(current_gid));
                    printf("global node id: %ld is owned by block: %d\n",
                           gid,
                           node_owner.getOwningBlock(gid));
                    PARFAIT_THROW(
                        "unable to assign global node ids.  Verify the block-to-block connectivity "
                        "is correct");
                }
                return;  // global node already set and valid
            }
            int expected_block_owner = -global_node_ids[block_id](i, j, k) - 1;
            if (expected_block_owner == node_owner.getOwningBlock(gid)) current_gid = gid;
            number_of_node_ids_set += 1;
        };

        syncer.sync(getId, setId, 0);
        need_sync = mp.ParallelSum(number_of_node_ids_set) > 0;
        number_of_node_ids_set = 0;
    }

    return {global_node_ids, node_owner};
}
StructuredMeshGlobalIds inf::assignStructuredMeshGlobalCellIds(
    const MessagePasser& mp, const StructuredBlockDimensions& cell_dimensions) {
    TRACER_PROFILE_FUNCTION
    std::vector<int> block_ids;
    for (const auto& p : cell_dimensions) block_ids.push_back(p.first);
    auto block_to_rank = buildBlockToRank(mp, block_ids);

    std::vector<int> block_counts;
    for (int block_id : block_ids) {
        const auto& d = cell_dimensions.at(block_id);
        block_counts.push_back(d[0] * d[1] * d[2]);
    }
    auto all_block_ids = mp.Gather(block_ids);
    auto all_block_counts = mp.Gather(block_counts);
    std::map<int, int> all_counts;
    for (int rank = 0; rank < mp.NumberOfProcesses(); ++rank) {
        const auto& ids = all_block_ids.at(rank);
        const auto& counts = all_block_counts.at(rank);
        for (size_t i = 0; i < all_block_ids.at(rank).size(); ++i) {
            all_counts[ids[i]] = counts[i];
        }
    }

    std::map<int, std::array<long, 2>> ownership_ranges;
    long index = 0;
    for (const auto& p : all_counts) {
        ownership_ranges[p.first].front() = index;
        index += p.second;
        ownership_ranges[p.first].back() = index - 1;
    }
    StructuredMeshGlobalIdOwner cell_owners(ownership_ranges, block_to_rank);

    StructuredBlockGlobalIds cell_ids;
    for (const auto& p : cell_dimensions) {
        int block_id = p.first;
        auto dimensions = p.second;
        cell_ids[block_id] = Vector3D<long>(dimensions);
        long gid = ownership_ranges.at(block_id).front();
        loopMDRange({0, 0, 0}, dimensions, [&](int i, int j, int k) {
            cell_ids[block_id](i, j, k) = gid++;
        });
    }
    return {cell_ids, cell_owners};
}
StructuredMeshGlobalIds inf::assignStructuredMeshGlobalCellIds(const MessagePasser& mp,
                                                               const StructuredMesh& mesh) {
    StructuredBlockDimensions cell_dimensions;
    for (int block_id : mesh.blockIds()) {
        cell_dimensions[block_id] = mesh.getBlock(block_id).nodeDimensions();
        for (int& d : cell_dimensions[block_id]) d -= 1;
    }
    return assignStructuredMeshGlobalCellIds(mp, cell_dimensions);
}
StructuredMeshGlobalIdOwner::StructuredMeshGlobalIdOwner(
    std::map<int, std::array<long, 2>> ownership_ranges, std::map<int, int> block_to_rank)
    : block_ownership_ranges(std::move(ownership_ranges)),
      block_to_rank(std::move(block_to_rank)) {}

int StructuredMeshGlobalIdOwner::getOwningBlock(long global_id) const {
    for (const auto& range : block_ownership_ranges) {
        auto lo = range.second.front();
        auto hi = range.second.back();
        if (global_id >= lo and global_id <= hi) return range.first;
    }

    PARFAIT_THROW("invalid global id: " + std::to_string(global_id));
}
int StructuredMeshGlobalIdOwner::getOwningRank(long global_id) const {
    return block_to_rank.at(getOwningBlock(global_id));
}
long StructuredMeshGlobalIdOwner::globalCount() const {
    long count = 0;
    for (const auto& range : block_ownership_ranges) {
        auto owned_by_block_count = range.second.back() - range.second.front() + 1;
        count += owned_by_block_count;
    }
    return count;
}
int StructuredMeshGlobalIdOwner::getOwningRankOfBlock(int block_id) const {
    return block_to_rank.at(block_id);
}

std::map<int, Vector3D<int>> assignUniqueIds(const StructuredMesh& mesh) {
    auto domain = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    for (int block_id : mesh.blockIds()) {
        const auto& block = mesh.getBlock(block_id);
        loopMDRange({0, 0, 0}, block.nodeDimensions(), [&](int i, int j, int k) {
            Parfait::ExtentBuilder::add(domain, block.point(i, j, k));
        });
    }
    Parfait::UniquePoints unique_points(domain);
    std::map<int, Vector3D<int>> ids;
    for (int block_id : mesh.blockIds()) {
        const auto& block = mesh.getBlock(block_id);
        ids[block_id] = Vector3D<int>(block.nodeDimensions());
        loopMDRange({0, 0, 0}, block.nodeDimensions(), [&](int i, int j, int k) {
            ids[block_id](i, j, k) = unique_points.getNodeId(block.point(i, j, k));
        });
    }
    return ids;
}

std::vector<int> assignBlockOwnership(const std::map<int, Vector3D<int>>& ids) {
    int max_global = 0;
    for (const auto& p : ids) {
        loopMDRange({0, 0, 0}, p.second.dimensions(), [&](int i, int j, int k) {
            max_global = std::max(max_global, p.second(i, j, k));
        });
    }
    std::vector<int> owned_by_block(max_global + 1, std::numeric_limits<int>::max());
    for (const auto& p : ids) {
        int block_id = p.first;
        for (int id : p.second.vec) {
            owned_by_block[id] = std::min(owned_by_block[id], block_id);
        }
    }
    return owned_by_block;
}

void reorderIdsToBeContiguousByBlock(std::map<int, Vector3D<int>>& ids,
                                     std::vector<int>& owned_by_block) {
    std::vector<int> old_to_new(owned_by_block.size());
    std::vector<bool> have_counted_global_id(owned_by_block.size(), false);
    int id = 0;
    for (const auto& p : ids) {
        int block_id = p.first;
        const auto& block = p.second;
        loopMDRange({0, 0, 0}, block.dimensions(), [&](int i, int j, int k) {
            if (owned_by_block[block(i, j, k)] == block_id and
                not have_counted_global_id[block(i, j, k)]) {
                old_to_new[block(i, j, k)] = id++;
                have_counted_global_id[block(i, j, k)] = true;
            }
        });
    }
    PARFAIT_ASSERT(id == (int)owned_by_block.size(),
                   "we should have assigned all node id an owner once");
    auto old_owned_by = owned_by_block;
    for (auto& p : ids) {
        Vector3D<int> new_ids(p.second.dimensions());
        loopMDRange({0, 0, 0}, new_ids.dimensions(), [&](int i, int j, int k) {
            auto old_id = p.second(i, j, k);
            new_ids(i, j, k) = old_to_new[old_id];
            PARFAIT_ASSERT_BOUNDS(
                owned_by_block, new_ids(i, j, k), "new block owner out of bounds");
            owned_by_block.at(new_ids(i, j, k)) = old_owned_by[old_id];
        });
        p.second = new_ids;
    }
}
std::map<int, std::array<long, 2>> buildOwnershipRanges(const std::map<int, Vector3D<int>>& ids,
                                                        const std::vector<int>& owned_by_block) {
    TRACER_PROFILE_FUNCTION
    std::map<int, std::array<long, 2>> ownership_ranges;
    for (const auto& p : ids) {
        int block_id = p.first;
        const auto& block = p.second;
        int range_start = std::numeric_limits<int>::max();
        int range_end = std::numeric_limits<int>::min();
        loopMDRange({0, 0, 0}, block.dimensions(), [&](int i, int j, int k) {
            auto id = block(i, j, k);
            if (owned_by_block[id] == block_id) {
                range_start = std::min(range_start, id);
                range_end = std::max(range_end, id);
            }
        });
        ownership_ranges[block_id] = {range_start, range_end};
    }
    return ownership_ranges;
}

inf::StructuredMeshGlobalIds inf::assignUniqueGlobalNodeIds(MessagePasser mp,
                                                            const StructuredMesh& mesh) {
    TRACER_PROFILE_FUNCTION
    auto block_to_rank = buildBlockToRank(mp, mesh.blockIds());

    auto all_blocks_rank_zero = block_to_rank;
    for (auto& p : all_blocks_rank_zero) p.second = 0;
    auto mesh_on_root = shuffleStructuredMesh(mp, mesh, all_blocks_rank_zero);

    std::map<int, MessagePasser::Message> assignments;
    std::map<int, std::array<long, 2>> ownership_ranges;
    if (mp.Rank() == 0) {
        auto ids = assignUniqueIds(*mesh_on_root);
        for (const auto& p : block_to_rank) {
            int block_id = p.first;
            int rank = p.second;
            assignments[rank].pack(block_id);
            assignments[rank].pack(ids.at(block_id).vec);
        }
        auto owned_by_block = assignBlockOwnership(ids);
        reorderIdsToBeContiguousByBlock(ids, owned_by_block);
        ownership_ranges = buildOwnershipRanges(ids, owned_by_block);
    }
    mp.Broadcast(ownership_ranges, 0);

    assignments = mp.Exchange(assignments);
    StructuredBlockGlobalIds ids;
    auto& m = assignments[0];
    for (size_t i = 0; i < mesh.blockIds().size(); ++i) {
        int block_id;
        m.unpack(block_id);
        Vector3D<int> ids_int(mesh.getBlock(block_id).nodeDimensions());
        m.unpack(ids_int.vec);
        ids[block_id] = Vector3D<long>(ids_int.dimensions());
        loopMDRange({0, 0, 0}, ids[block_id].dimensions(), [&](int i, int j, int k) {
            ids[block_id](i, j, k) = ids_int(i, j, k);
        });
    }
    StructuredMeshGlobalIdOwner owners(ownership_ranges, block_to_rank);
    return {ids, owners};
}
