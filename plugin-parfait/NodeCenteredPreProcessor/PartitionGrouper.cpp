#include "PartitionGrouper.h"

namespace PG {

std::shared_ptr<inf::TinfMesh> PartitionGrouper::group(MessagePasser mp,
                                                       const inf::MeshInterface& mesh,
                                                       const int num_partitions_per_group) const {
    auto graph = inf::PartitionConnectivityMapper::buildParallelPartToPart(mesh, mp);
    auto parmetis_output = generateGroupedPartVector(mp, graph, num_partitions_per_group);
    std::vector<int> collected_part_vector;
    mp.Gather(parmetis_output[0], collected_part_vector);
    auto finalParts =
        inf::PartitionConnectivityMapper::buildOldPartToNew(collected_part_vector, num_partitions_per_group);
    std::vector<int> new_node_owners(mesh.nodeCount());
    for (int i = 0; i < mesh.nodeCount(); ++i) {
        new_node_owners[i] = finalParts[mesh.nodeOwner(i)];
    }
    return inf::MeshShuffle::shuffleNodes(mp, mesh, new_node_owners);
}
}  // namespace PG
