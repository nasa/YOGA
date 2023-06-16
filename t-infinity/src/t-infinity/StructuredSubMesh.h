#pragma once
#include <map>
#include "StructuredMesh.h"

namespace inf {
class StructuredSubBlock : public StructuredBlock {
  public:
    struct NodeRange {
        int block_id;
        std::array<int, 3> min;
        std::array<int, 3> max;
    };

    struct BlockIJK {
        int block_id;
        int i;
        int j;
        int k;
    };

    StructuredSubBlock() = default;
    StructuredSubBlock(int block_id,
                       std::shared_ptr<StructuredMesh> original_mesh,
                       NodeRange original_node_range)
        : block_id(block_id),
          original_mesh(std::move(original_mesh)),
          original_node_range(original_node_range) {}

    std::array<int, 3> nodeDimensions() const override {
        return {original_node_range.max[0] - original_node_range.min[0],
                original_node_range.max[1] - original_node_range.min[1],
                original_node_range.max[2] - original_node_range.min[2]};
    }
    Parfait::Point<double> point(int i, int j, int k) const override {
        i += original_node_range.min[0];
        j += original_node_range.min[1];
        k += original_node_range.min[2];
        const auto& original_block = original_mesh->getBlock(original_node_range.block_id);
        return original_block.point(i, j, k);
    }
    void setPoint(int i, int j, int k, const Parfait::Point<double>& p) override {
        i += original_node_range.min[0];
        j += original_node_range.min[1];
        k += original_node_range.min[2];
        auto& original_block = original_mesh->getBlock(original_node_range.block_id);
        original_block.setPoint(i, j, k, p);
    }
    int blockId() const override { return block_id; }

    BlockIJK previousNodeBlockIJK(int i, int j, int k) const {
        i += original_node_range.min[0];
        j += original_node_range.min[1];
        k += original_node_range.min[2];
        return {original_node_range.block_id, i, j, k};
    }

  private:
    int block_id;
    std::shared_ptr<StructuredMesh> original_mesh;
    NodeRange original_node_range;
};

class StructuredSubMesh : public StructuredMesh {
  public:
    using NodeRanges = std::map<int, StructuredSubBlock::NodeRange>;
    StructuredSubMesh(std::shared_ptr<StructuredMesh> mesh, const NodeRanges& ranges)
        : original_mesh(std::move(mesh)) {
        for (const auto& p : ranges) {
            subblocks[p.first] = StructuredSubBlock(p.first, original_mesh, p.second);
        }
    }
    std::vector<int> blockIds() const override {
        std::vector<int> block_ids;
        for (const auto& p : subblocks) block_ids.push_back(p.first);
        return block_ids;
    }
    StructuredSubBlock& getBlock(int block_id) override { return subblocks[block_id]; }
    const StructuredSubBlock& getBlock(int block_id) const override {
        return subblocks.at(block_id);
    }

  private:
    std::shared_ptr<StructuredMesh> original_mesh;
    std::map<int, StructuredSubBlock> subblocks;
};
}
