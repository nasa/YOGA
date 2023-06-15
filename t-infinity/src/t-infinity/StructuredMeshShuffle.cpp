#include "StructuredMeshShuffle.h"
#include <t-infinity/SMesh.h>

using namespace inf;

void packBlock(MessagePasser::Message& msg, std::shared_ptr<StructuredBlock> block) {
    msg.pack(block->blockId());
    msg.pack(block->nodeDimensions());
    loopMDRange({0, 0, 0}, block->nodeDimensions(), [&](int i, int j, int k) {
        msg.pack(block->point(i, j, k));
    });
}
void unpackBlock(MessagePasser::Message& msg, std::shared_ptr<StructuredBlock>& b) {
    int block_id = -1;
    std::array<int, 3> node_dimensions;
    msg.unpack(block_id);
    msg.unpack(node_dimensions);
    auto block = SMesh::createBlock(node_dimensions, block_id);
    loopMDRange({0, 0, 0}, node_dimensions, [&](int i, int j, int k) {
        msg.unpack(block->points(i, j, k));
    });
    b = block;
}
std::shared_ptr<StructuredMesh> inf::shuffleStructuredMesh(
    MessagePasser mp, const StructuredMesh& mesh, const std::map<int, int>& block_to_rank) {
    using SBlock = std::shared_ptr<StructuredBlock>;
    std::map<int, std::vector<SBlock>> messages;
    for (int block_id : mesh.blockIds()) {
        int dest_rank = block_to_rank.at(block_id);
        auto block = std::make_shared<SMesh::Block>(mesh.getBlock(block_id));
        messages[dest_rank].push_back(block);
    }
    auto out_blocks = mp.Exchange(messages, packBlock, unpackBlock);

    auto new_mesh = std::make_shared<SMesh>();
    for (auto& b : out_blocks) {
        for (auto& m : b.second) {
            new_mesh->add(m);
        }
    }

    return new_mesh;
}
Parfait::SyncPattern inf::buildStructuredMeshtoMeshSyncPattern(
    const MessagePasser& mp,
    const StructuredMeshGlobalIds& source,
    const StructuredMeshGlobalIds& target) {
    auto extractGlobalIds = [&](const StructuredMeshGlobalIds& global_ids) -> std::vector<long> {
        std::set<long> have;
        for (const auto& p : global_ids.ids) {
            const auto& gids = p.second;
            loopMDRange({0, 0, 0}, gids.dimensions(), [&](int i, int j, int k) {
                long gid = gids(i, j, k);
                if (gid >= 0)
                    if (global_ids.owner.getOwningRank(gid) == mp.Rank()) have.insert(gid);
            });
        }
        return {have.begin(), have.end()};
    };
    auto have = extractGlobalIds(source);
    auto need = extractGlobalIds(target);
    return {mp, have, need};
}

StructuredMeshGlobalIds inf::shuffleStructuredMeshGlobalIds(
    MessagePasser mp,
    const StructuredMeshGlobalIds& global,
    const std::map<int, int>& block_to_rank) {
    std::vector<int> number_of_blocks_per_dest_rank(mp.NumberOfProcesses(), 0);
    for (const auto& p : global.ids) {
        int dest_rank = block_to_rank.at(p.first);
        number_of_blocks_per_dest_rank[dest_rank]++;
    }

    std::map<int, MessagePasser::Message> messages;
    for (int rank = 0; rank < mp.NumberOfProcesses(); ++rank) {
        int count = number_of_blocks_per_dest_rank.at(rank);
        if (count > 0) messages[rank].pack(count);
    }

    for (const auto& p : global.ids) {
        int block_id = p.first;
        const auto& ids = p.second;
        int dest_rank = block_to_rank.at(block_id);
        auto& msg = messages[dest_rank];
        msg.pack(block_id);
        msg.pack(ids.dimensions());
        msg.pack(ids.vec);
    }
    auto result = mp.Exchange(messages);

    StructuredBlockGlobalIds new_global_ids;
    for (auto& p : result) {
        auto& msg = p.second;
        int number_of_blocks_in_message = -1;
        msg.unpack(number_of_blocks_in_message);
        for (int i = 0; i < number_of_blocks_in_message; ++i) {
            int block_id;
            std::array<int, 3> dimensions{};
            msg.unpack(block_id);
            msg.unpack(dimensions);
            new_global_ids[block_id] = Vector3D<long>(dimensions);
            msg.unpack(new_global_ids[block_id].vec);
        }
    }

    StructuredMeshGlobalIdOwner new_owners(global.owner.getOwnershipRanges(), block_to_rank);

    return {new_global_ids, new_owners};
}
