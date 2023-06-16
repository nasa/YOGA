#pragma once
#include <MessagePasser/MessagePasser.h>
#include <vector>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/MeshShuffle.h>
#include "../shared/ParmetisWrapper.h"
#include <t-infinity/PartitionConnectivityMapper.h>
#include "GraphBuilder.h"

namespace PG {

template <typename Graph>
std::vector<int> generateGroupedPartVector(MessagePasser mp,
                                           const Graph& node_to_node,
                                           const int num_partitions_per_group) {
    idx_t target_number_of_partitions = mp.NumberOfProcesses() / num_partitions_per_group;
    auto part = Parfait::ParmetisWrapper::generatePartVector(mp, node_to_node, target_number_of_partitions);
    return part;
}

class PartitionGrouper {
  public:
    std::shared_ptr<inf::TinfMesh> group(MessagePasser mp,
                                         const inf::MeshInterface& nm,
                                         const int num_partitions_per_group) const;
};

}  // namespace PG
