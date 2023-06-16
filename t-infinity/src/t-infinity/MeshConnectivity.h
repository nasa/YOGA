#pragma once
#include "MeshInterface.h"
#include <set>
#include <vector>

namespace inf {
namespace NodeToNode {
    std::vector<std::vector<int>> buildForAnyNodeInSharedCell(const inf::MeshInterface& mesh);
    std::vector<std::vector<int>> build(const inf::MeshInterface& mesh);
    std::vector<std::vector<int>> buildSurfaceOnly(const inf::MeshInterface& mesh);
    std::vector<std::array<int, 2>> buildUniqueSurfaceEdgesForCellsTouchingNodes(
        const inf::MeshInterface& mesh, const std::set<int>& requested_nodes);
}
namespace NodeToCell {
    std::vector<std::vector<int>> build(const inf::MeshInterface& mesh);
    std::vector<std::vector<int>> buildDimensionOnly(const inf::MeshInterface& mesh, int dimension);
    std::vector<std::vector<int>> buildVolumeOnly(const inf::MeshInterface& mesh);
    std::vector<std::vector<int>> buildSurfaceOnly(const inf::MeshInterface& mesh);
}
namespace CellToCell {
    std::vector<std::vector<int>> build(const inf::MeshInterface& mesh,
                                        const std::vector<std::vector<int>>& n2c);
    std::vector<std::vector<int>> build(const inf::MeshInterface& mesh);
    std::vector<std::vector<int>> buildDimensionOnly(const inf::MeshInterface& mesh, int dimension);
    std::vector<std::vector<int>> buildDimensionOnly(const inf::MeshInterface& mesh,
                                                     const std::vector<std::vector<int>>& n2c,
                                                     int dimension);
}
namespace EdgeToCell {
    std::vector<std::vector<int>> build(const inf::MeshInterface& mesh,
                                        const std::vector<std::array<int, 2>>& edges);
}
namespace EdgeToCellNodes {
    std::vector<std::vector<int>> build(const inf::MeshInterface& mesh,
                                        const std::vector<std::array<int, 2>>& edges);
}
namespace EdgeToNode {
    std::vector<std::array<int, 2>> build(const MeshInterface& mesh);
    std::vector<std::array<int, 2>> buildOnSurface(const MeshInterface& mesh,
                                                   std::set<int> surface_tags);
}
}
