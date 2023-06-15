#pragma once
#include <vector>
#include "MeshInterface.h"
#include "MessagePasser/MessagePasser.h"

namespace inf {
class TinfMesh;

namespace PartitionDiffusion {
    std::vector<int> reassignCellOwners(const inf::MeshInterface& mesh,
                                        const std::vector<std::vector<int>>& neighbors);

    std::vector<std::vector<int>> addLayer(const std::vector<std::vector<int>>& graph);

    std::shared_ptr<TinfMesh> cellBasedDiffusion(MessagePasser mp,
                                                 std::shared_ptr<inf::MeshInterface> mesh,
                                                 int num_sweeps);
}
}
