#pragma once
#include <memory>
#include <utility>
#include "StructuredMeshGlobalIds.h"
#include "StructuredMesh.h"
#include "StructuredMeshPartitioner.h"

namespace inf {
class PartitionedStructuredMesh {
  public:
    PartitionedStructuredMesh(const MessagePasser& mp,
                              const StructuredMesh& mesh,
                              StructuredMeshGlobalIds global_node_in);
    std::shared_ptr<StructuredMesh> getMesh() const;
    const StructuredMeshGlobalIds& getGlobalNodeIds() const;
    const StructuredMeshGlobalIds& getGlobalCellIds() const;
    long previousGlobalNodeId(long global_node_id) const;
    long previousGlobalCellId(long global_cell_id) const;
    long globalNodeCount() const;
    long globalCellCount() const;
    double getCurrentLoadBalance() const;
    void balance(double target_load_balance = 0.9);

  private:
    MessagePasser mp;
    std::shared_ptr<StructuredMesh> partitioned_mesh;
    StructuredMeshGlobalIds global_node;
    StructuredMeshGlobalIds global_cell;
    StructuredMeshPartitioner::BlockMappings block_mappings;
    std::map<long, long> new_to_old_global_node_id;
    std::map<long, long> new_to_old_global_cell_id;

    std::map<long, long> buildNewToOldGlobalIds(const StructuredBlockGlobalIds& new_global_ids,
                                                const StructuredBlockGlobalIds& old_global_ids);
    std::map<int, double> calcCellsPerBlock() const;
    void splitMesh();
    void shuffleBlockMappings(const std::map<int, int>& block_to_rank);
};
}
