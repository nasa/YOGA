#pragma once
#include <t-infinity/StructuredMeshGlobalIds.h>

namespace inf {
class SplitStructuredBlockMapping {
  public:
    SplitStructuredBlockMapping();
    SplitStructuredBlockMapping(int original_block_id, const StructuredMeshGlobalIds& global_node);

    std::array<SplitStructuredBlockMapping, 2> split(BlockAxis split_axis) const;
    std::array<int, 3> nodeDimensions() const;
    int nodeCount() const;
    int cellCount() const;
    bool isOwnedNode(int i, int j, int k) const;
    int ownedCount() const;

    int original_block_id;
    std::array<int, 3> original_node_range_min;
    std::array<int, 3> original_node_range_max;
    Vector3D<int> owned_by_block;

    void pack(MessagePasser::Message& msg) const;

    static SplitStructuredBlockMapping unpack(MessagePasser::Message& msg);

  private:
    std::tuple<std::array<int, 3>, std::array<int, 3>> splitNodeDimensions(
        BlockAxis split_axis) const;

    Vector3D<int> buildOwnedByBlock(const StructuredMeshGlobalIds& global_node) const;
};

}
