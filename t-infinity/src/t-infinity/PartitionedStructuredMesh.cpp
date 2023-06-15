#include "PartitionedStructuredMesh.h"
#include "SMesh.h"
#include "StructuredMeshShuffle.h"
#include "StructuredMeshLoadBalancing.h"

inf::PartitionedStructuredMesh::PartitionedStructuredMesh(
    const MessagePasser& mp,
    const inf::StructuredMesh& mesh,
    inf::StructuredMeshGlobalIds global_node_in)
    : mp(mp),
      partitioned_mesh(std::make_shared<SMesh>(mesh)),
      global_node(std::move(global_node_in)),
      global_cell(assignStructuredMeshGlobalCellIds(mp, mesh)),
      block_mappings(StructuredMeshPartitioner::buildBlockMappings(global_node)),
      new_to_old_global_node_id(buildNewToOldGlobalIds(global_node.ids, global_node.ids)),
      new_to_old_global_cell_id(buildNewToOldGlobalIds(global_cell.ids, global_cell.ids)) {}

std::shared_ptr<inf::StructuredMesh> inf::PartitionedStructuredMesh::getMesh() const {
    return partitioned_mesh;
}

long inf::PartitionedStructuredMesh::globalNodeCount() const {
    return global_node.owner.globalCount();
}
long inf::PartitionedStructuredMesh::globalCellCount() const {
    return global_cell.owner.globalCount();
}

double inf::PartitionedStructuredMesh::getCurrentLoadBalance() const {
    double cells_on_rank = 0;
    for (const auto& p : block_mappings) {
        auto d = p.second.nodeDimensions();
        cells_on_rank += (d[0] - 1) * (d[1] - 1) * (d[2] - 1);
    }
    auto cells_per_rank = mp.Gather(cells_on_rank);
    return measureLoadBalance(cells_per_rank);
}

std::map<int, double> inf::PartitionedStructuredMesh::calcCellsPerBlock() const {
    std::vector<std::array<int, 2>> local_block_to_cell_count;
    for (const auto& p : block_mappings)
        local_block_to_cell_count.push_back({p.first, p.second.cellCount()});
    auto block_to_cell = mp.Gather(local_block_to_cell_count);
    std::map<int, double> all_block_cell_counts;
    for (int rank = 0; rank < mp.NumberOfProcesses(); ++rank) {
        for (const auto& b2c : block_to_cell.at(rank)) {
            all_block_cell_counts[b2c.front()] = b2c.back();
        }
    }
    return all_block_cell_counts;
}

void inf::PartitionedStructuredMesh::balance(double target_load_balance) {
    auto load_balance = getCurrentLoadBalance();
    while (load_balance < target_load_balance) {
        StructuredMeshPartitioner::splitLargestBlock(mp, block_mappings);
        auto cells_per_block = calcCellsPerBlock();
        auto block_to_rank = mapWorkToRanks(cells_per_block, mp.NumberOfProcesses());
        std::vector<double> cells_on_rank(mp.NumberOfProcesses(), 0.0);
        for (const auto p : block_to_rank) cells_on_rank[p.second] += cells_per_block.at(p.first);
        load_balance = measureLoadBalance(cells_on_rank);
    }
    splitMesh();
}
void inf::PartitionedStructuredMesh::splitMesh() {
    SMesh split_mesh;
    for (const auto& p : block_mappings) {
        int block_id = p.first;
        const auto& block = p.second;

        auto block_start = block.original_node_range_min;
        auto block_end = block.original_node_range_max;

        auto new_block = inf::SMesh::createBlock(block.nodeDimensions(), block_id);
        loopMDRange(block_start, block_end, [&](int i, int j, int k) {
            int inew = i - block_start[0];
            int jnew = j - block_start[1];
            int knew = k - block_start[2];
            new_block->points(inew, jnew, knew) =
                partitioned_mesh->getBlock(block.original_block_id).point(i, j, k);
        });
        split_mesh.add(new_block);
    }
    auto cells_per_block = calcCellsPerBlock();
    auto block_to_rank = mapWorkToRanks(cells_per_block, mp.NumberOfProcesses());

    global_node.ids = StructuredMeshPartitioner::splitGlobalNodes(block_mappings, global_node.ids);
    global_cell.ids = StructuredMeshPartitioner::splitGlobalCells(block_mappings, global_cell.ids);

    shuffleBlockMappings(block_to_rank);
    partitioned_mesh = shuffleStructuredMesh(mp, split_mesh, block_to_rank);
    global_node = shuffleStructuredMeshGlobalIds(mp, global_node, block_to_rank);
    global_cell = shuffleStructuredMeshGlobalIds(mp, global_cell, block_to_rank);
    auto original_global_node_ids = global_node.ids;
    auto original_global_cell_ids = global_cell.ids;

    global_node.ids = StructuredMeshPartitioner::makeGlobalNodeIdsContiguousByBlock(
        global_node.ids, mp, block_mappings);
    global_node.owner = StructuredMeshPartitioner::buildSplitBlockOwnership(mp, block_mappings);

    new_to_old_global_node_id = buildNewToOldGlobalIds(global_node.ids, original_global_node_ids);
    new_to_old_global_cell_id = buildNewToOldGlobalIds(global_cell.ids, original_global_cell_ids);
}
void inf::PartitionedStructuredMesh::shuffleBlockMappings(const std::map<int, int>& block_to_rank) {
    std::map<int, std::vector<int>> block_ids_to_exchange;
    std::map<int, std::vector<SplitStructuredBlockMapping>> blocks_to_exchange;
    for (const auto& p : block_mappings) {
        int block_id = p.first;
        const auto& block = p.second;
        int dest_rank = block_to_rank.at(block_id);
        block_ids_to_exchange[dest_rank].push_back(block_id);
        blocks_to_exchange[dest_rank].push_back(block);
    }
    auto pack_blocks = [](MessagePasser::Message& msg, const SplitStructuredBlockMapping& b) {
        b.pack(msg);
    };
    auto unpack_blocks = [](MessagePasser::Message& msg, SplitStructuredBlockMapping& b) {
        b = SplitStructuredBlockMapping::unpack(msg);
    };
    block_ids_to_exchange = mp.Exchange(block_ids_to_exchange);
    blocks_to_exchange = mp.Exchange(blocks_to_exchange, pack_blocks, unpack_blocks);
    block_mappings.clear();
    for (const auto& p : block_ids_to_exchange) {
        int rank = p.first;
        PARFAIT_ASSERT(blocks_to_exchange.at(rank).size() == block_ids_to_exchange.at(rank).size(),
                       "Size mismatch");
        for (size_t i = 0; i < block_ids_to_exchange.size(); ++i) {
            int block_id = block_ids_to_exchange.at(rank).at(i);
            block_mappings[block_id] = blocks_to_exchange.at(rank).at(i);
        }
    }
}
const inf::StructuredMeshGlobalIds& inf::PartitionedStructuredMesh::getGlobalNodeIds() const {
    return global_node;
}
const inf::StructuredMeshGlobalIds& inf::PartitionedStructuredMesh::getGlobalCellIds() const {
    return global_cell;
}
long inf::PartitionedStructuredMesh::previousGlobalNodeId(long global_node_id) const {
    return new_to_old_global_node_id.at(global_node_id);
}
long inf::PartitionedStructuredMesh::previousGlobalCellId(long global_cell_id) const {
    return new_to_old_global_cell_id.at(global_cell_id);
}
std::map<long, long> inf::PartitionedStructuredMesh::buildNewToOldGlobalIds(
    const inf::StructuredBlockGlobalIds& new_global_ids,
    const inf::StructuredBlockGlobalIds& old_global_ids) {
    std::map<long, long> new_to_old;
    for (const auto& p : new_global_ids) {
        for (size_t i = 0; i < p.second.vec.size(); ++i) {
            long new_gid = p.second.vec[i];
            long old_gid = old_global_ids.at(p.first).vec[i];
            new_to_old[new_gid] = old_gid;
        }
    }
    return new_to_old;
}
