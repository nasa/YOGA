#pragma once
#include "StructuredSubMesh.h"
#include "StructuredBlockConnectivity.h"
#include "StructuredMeshHelpers.h"

namespace inf {
class StructuredNodeRangeSelector {
  public:
    virtual StructuredSubMesh::NodeRanges getRanges(const StructuredMesh& mesh) const = 0;
    virtual ~StructuredNodeRangeSelector() = default;
};

class StructuredBlockSideSelector : public StructuredNodeRangeSelector {
  public:
    using BlockSidesToFilter = std::map<int, std::set<BlockSide>>;
    StructuredBlockSideSelector(MessagePasser mp, BlockSidesToFilter block_sides_to_filter)
        : mp(mp), block_sides_to_filter(std::move(block_sides_to_filter)) {}
    StructuredSubMesh::NodeRanges getRanges(const StructuredMesh& mesh) const override {
        auto block_to_rank = buildBlockToRank(mp, mesh.blockIds());
        StructuredSubMesh::NodeRanges ranges;
        int next_block_id = 0;
        for (int rank = 0; rank < mp.NumberOfProcesses(); ++rank) {
            if (mp.Rank() == rank) {
                for (const auto& p : block_sides_to_filter) {
                    if (block_to_rank.at(p.first) != rank) continue;
                    const auto& block = mesh.getBlock(p.first);
                    auto node_dimensions = block.nodeDimensions();
                    for (BlockSide side : p.second) {
                        auto& range = ranges[next_block_id++];
                        range.block_id = block.blockId();
                        range.min = {0, 0, 0};
                        range.max = node_dimensions;
                        auto axis = sideToAxis(side);
                        if (isMaxFace(side)) {
                            range.min[axis] = node_dimensions[axis] - 1;
                        } else {
                            range.max[axis] = 1;
                        }
                    }
                }
            }
            next_block_id = mp.ParallelMax(next_block_id);
        }
        return ranges;
    }

  private:
    MessagePasser mp;
    BlockSidesToFilter block_sides_to_filter;
};
}
