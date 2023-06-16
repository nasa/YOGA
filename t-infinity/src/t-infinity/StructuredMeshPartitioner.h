#pragma once
#include "SplitStructuredBlockMapping.h"

namespace inf {
namespace StructuredMeshPartitioner {
    using BlockMappings = std::map<int, SplitStructuredBlockMapping>;
    inf::StructuredMeshPartitioner::BlockMappings buildBlockMappings(
        const inf::StructuredMeshGlobalIds& global_node);
    StructuredBlockGlobalIds splitGlobalNodes(const BlockMappings& blocks,
                                              const StructuredBlockGlobalIds& global_node);
    StructuredBlockGlobalIds splitGlobalCells(const BlockMappings& blocks,
                                              const StructuredBlockGlobalIds& global_cell);
    StructuredMeshGlobalIdOwner buildSplitBlockOwnership(const MessagePasser& mp,
                                                         const BlockMappings& blocks);
    StructuredMeshGlobalIds buildNewGlobalNodes(const MessagePasser& mp,
                                                const BlockMappings& blocks,
                                                const StructuredMeshGlobalIds& global_node);
    StructuredBlockGlobalIds makeGlobalNodeIdsContiguousByBlock(
        const StructuredBlockGlobalIds& global_node_ids,
        const MessagePasser& mp,
        const BlockMappings& blocks);
    std::map<int, int> buildBlockOwnedCounts(const MessagePasser& mp, const BlockMappings& blocks);
    void splitLargestBlock(const MessagePasser& mp, BlockMappings& blocks);
    int getNextAvailableBlockId(const MessagePasser& mp, const BlockMappings& blocks);
    int findBlockWithLargestNumberOfNodes(const BlockMappings& blocks);
    BlockAxis findLargestAxis(const std::array<int, 3>& node_dimensions);
}
}