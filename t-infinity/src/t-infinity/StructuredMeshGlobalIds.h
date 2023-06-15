#pragma once

#include <MessagePasser/MessagePasser.h>
#include "MDVector.h"
#include "StructuredMeshConnectivity.h"
#include <parfait/Throw.h>

namespace inf {
class StructuredMeshGlobalIdOwner {
  public:
    StructuredMeshGlobalIdOwner(std::map<int, std::array<long, 2>> ownership_ranges,
                                std::map<int, int> block_to_rank);

    int getOwningBlock(long global_id) const;
    int getOwningRank(long global_id) const;
    long globalCount() const;
    int getOwningRankOfBlock(int block_id) const;
    inline std::map<int, std::array<long, 2>> getOwnershipRanges() const {
        return block_ownership_ranges;
    }

  private:
    std::map<int, std::array<long, 2>> block_ownership_ranges;
    std::map<int, int> block_to_rank;
};

using StructuredBlockDimensions = std::map<int, std::array<int, 3>>;
using StructuredBlockGlobalIds = std::map<int, Vector3D<long>>;

struct StructuredMeshGlobalIds {
    StructuredBlockGlobalIds ids;
    StructuredMeshGlobalIdOwner owner;
};

StructuredMeshGlobalIds assignStructuredMeshGlobalNodeIds(
    const MessagePasser& mp,
    const MeshConnectivity& mesh_connectivity,
    const StructuredBlockDimensions& node_dimensions);
StructuredMeshGlobalIds assignStructuredMeshGlobalNodeIds(const MessagePasser& mp,
                                                          const MeshConnectivity& mesh_connectivity,
                                                          const StructuredMesh& mesh);
StructuredMeshGlobalIds assignStructuredMeshGlobalCellIds(
    const MessagePasser& mp, const StructuredBlockDimensions& cell_dimensions);
StructuredMeshGlobalIds assignStructuredMeshGlobalCellIds(const MessagePasser& mp,
                                                          const StructuredMesh& mesh);

StructuredMeshGlobalIds assignUniqueGlobalNodeIds(MessagePasser mp, const StructuredMesh& mesh);
}
