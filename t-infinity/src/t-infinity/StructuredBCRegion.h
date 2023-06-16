#pragma once
#include "StructuredBlockFace.h"
#include "StructuredTinfMesh.h"

namespace inf {
struct StructuredBCRegion {
    enum { MIN = -2, MAX = -1 };

    std::string name;
    int host_block;
    BlockSide side;

    BlockAxis axis1;
    std::array<int, 2> axis1_range;

    BlockAxis axis2;
    std::array<int, 2> axis2_range;

    std::array<int, 3> start(const std::array<int, 3>& node_dimensions) const;
    std::array<int, 3> end(const std::array<int, 3>& node_dimensions) const;

    BlockAxis getFaceAxis() const;
    std::string to_string() const;

    static BlockAxis axisFromString(std::string IorJorK);
    static int rangeIntFromString(std::string s);
    static inf::BlockSide sideFromString(std::string side_string);
    static std::vector<StructuredBCRegion> parse(std::vector<std::string> lines);
    static StructuredBCRegion parse(std::string line);
    static void reassignBoundaryTagsFromBCRegions(
        inf::StructuredTinfMesh& mesh, const std::vector<StructuredBCRegion>& bc_regions);
};
}