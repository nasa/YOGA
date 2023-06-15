#include "SplitStructuredBlockMapping.h"
#include <algorithm>

inf::SplitStructuredBlockMapping::SplitStructuredBlockMapping()
    : original_block_id(-1),
      original_node_range_min({}),
      original_node_range_max({}),
      owned_by_block(nodeDimensions()) {}
inf::SplitStructuredBlockMapping::SplitStructuredBlockMapping(
    int original_block_id, const inf::StructuredMeshGlobalIds& global_node)
    : original_block_id(original_block_id),
      original_node_range_min({0, 0, 0}),
      original_node_range_max(global_node.ids.at(original_block_id).dimensions()),
      owned_by_block(buildOwnedByBlock(global_node)) {}
std::array<inf::SplitStructuredBlockMapping, 2> inf::SplitStructuredBlockMapping::split(
    inf::BlockAxis split_axis) const {
    const auto& block = *this;
    SplitStructuredBlockMapping lo_block, hi_block;
    std::array<int, 3> lo_dimensions, hi_dimensions;
    std::tie(lo_dimensions, hi_dimensions) = splitNodeDimensions(split_axis);

    lo_block.original_block_id = block.original_block_id;
    lo_block.original_node_range_min = block.original_node_range_min;
    lo_block.original_node_range_max = block.original_node_range_max;
    lo_block.original_node_range_max[split_axis] =
        lo_block.original_node_range_min[split_axis] + lo_dimensions[split_axis];

    lo_block.owned_by_block = Vector3D<int>(lo_block.nodeDimensions());
    loopMDRange({0, 0, 0}, lo_block.owned_by_block.dimensions(), [&](int i, int j, int k) {
        lo_block.owned_by_block(i, j, k) = block.owned_by_block(i, j, k);
    });

    hi_block.original_block_id = block.original_block_id;
    hi_block.original_node_range_min = block.original_node_range_min;
    hi_block.original_node_range_max = block.original_node_range_max;
    hi_block.original_node_range_min[split_axis] =
        hi_block.original_node_range_max[split_axis] - hi_dimensions[split_axis];

    std::array<int, 3> offset{};
    offset[split_axis] = lo_dimensions[split_axis] - 1;

    hi_block.owned_by_block = Vector3D<int>(hi_block.nodeDimensions());
    loopMDRange({0, 0, 0}, hi_block.owned_by_block.dimensions(), [&](int i, int j, int k) {
        hi_block.owned_by_block(i, j, k) =
            block.owned_by_block(i + offset[0], j + offset[1], k + offset[2]);
    });

    // Convention: The low block in the split always retains ownership the shared nodes in a split.
    // Set all nodes on the high block side connected to the low block to "not owned".
    auto lo_side_of_hi_block = hi_dimensions;
    lo_side_of_hi_block[split_axis] = 1;
    loopMDRange({0, 0, 0}, lo_side_of_hi_block, [&](int i, int j, int k) {
        hi_block.owned_by_block(i, j, k) = 0;
    });

    return {lo_block, hi_block};
}
std::array<int, 3> inf::SplitStructuredBlockMapping::nodeDimensions() const {
    return {original_node_range_max[0] - original_node_range_min[0],
            original_node_range_max[1] - original_node_range_min[1],
            original_node_range_max[2] - original_node_range_min[2]};
}
bool inf::SplitStructuredBlockMapping::isOwnedNode(int i, int j, int k) const {
    return owned_by_block(i, j, k);
}

std::tuple<std::array<int, 3>, std::array<int, 3>>
inf::SplitStructuredBlockMapping::splitNodeDimensions(inf::BlockAxis split_axis) const {
    auto node_dimensions = nodeDimensions();
    std::array<int, 3> lo = node_dimensions, hi = node_dimensions;
    int count_in_split = node_dimensions[split_axis] - 1;
    if (count_in_split % 2 == 0) {
        lo[split_axis] = count_in_split / 2 + 1;
        hi[split_axis] = count_in_split / 2 + 1;
    } else {
        lo[split_axis] = node_dimensions[split_axis] / 2 + 1;
        hi[split_axis] = node_dimensions[split_axis] / 2;
    }
    return {lo, hi};
}
inf::Vector3D<int> inf::SplitStructuredBlockMapping::buildOwnedByBlock(
    const inf::StructuredMeshGlobalIds& global_node) const {
    Vector3D<int> do_own(nodeDimensions());
    const auto& global_ids = global_node.ids.at(original_block_id);
    loopMDRange(original_node_range_min, original_node_range_max, [&](int i, int j, int k) {
        do_own(i, j, k) =
            global_node.owner.getOwningBlock(global_ids(i, j, k)) == original_block_id ? 1 : 0;
    });
    return do_own;
}
int inf::SplitStructuredBlockMapping::ownedCount() const {
    return std::count(owned_by_block.vec.begin(), owned_by_block.vec.end(), 1);
}
int inf::SplitStructuredBlockMapping::nodeCount() const {
    int node_count = 1;
    for (int d : nodeDimensions()) node_count *= d;
    return node_count;
}
int inf::SplitStructuredBlockMapping::cellCount() const {
    int cell_count = 1;
    for (int d : nodeDimensions()) cell_count *= (d - 1);
    return cell_count;
}
void inf::SplitStructuredBlockMapping::pack(MessagePasser::Message& msg) const {
    msg.pack(original_block_id);
    msg.pack(original_node_range_min);
    msg.pack(original_node_range_max);
    msg.pack(owned_by_block.dimensions());
    msg.pack(owned_by_block.vec);
}
inf::SplitStructuredBlockMapping inf::SplitStructuredBlockMapping::unpack(
    MessagePasser::Message& msg) {
    SplitStructuredBlockMapping block;
    msg.unpack(block.original_block_id);
    msg.unpack(block.original_node_range_min);
    msg.unpack(block.original_node_range_max);
    std::array<int, 3> dimensions{};
    msg.unpack(dimensions);
    block.owned_by_block = Vector3D<int>(dimensions);
    msg.unpack(block.owned_by_block.vec);
    return block;
}
