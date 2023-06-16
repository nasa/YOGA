#pragma once
#include "TinfMesh.h"
#include "StructuredMeshGlobalIds.h"
#include "GlobalToLocal.h"

namespace inf {
class StructuredTinfMesh : public TinfMesh {
  public:
    using LocalIds = std::map<int, Vector3D<int>>;
    StructuredTinfMesh(const TinfMeshData& data,
                       int partition_id,
                       LocalIds local_nodes,
                       LocalIds local_cells);
    StructuredTinfMesh(const TinfMeshData& data,
                       int partition_id,
                       const StructuredBlockGlobalIds& global_nodes,
                       const StructuredBlockGlobalIds& global_cells);
    StructuredTinfMesh(const TinfMesh& mesh,
                       const StructuredBlockGlobalIds& global_nodes,
                       const StructuredBlockGlobalIds& global_cells);

    int cellId(int block_id, int i, int j, int k) const;
    int nodeId(int block_id, int i, int j, int k) const;
    std::vector<int> blockIds() const;

    StructuredBlock& getBlock(int block_id);
    const StructuredBlock& getBlock(int block_id) const;
    std::shared_ptr<StructuredBlockField> getField(int block_id, const FieldInterface& field) const;
    std::shared_ptr<StructuredField> getField(const FieldInterface& field) const;
    std::shared_ptr<StructuredMesh> shallowCopyToStructuredMesh() const;
    std::shared_ptr<StructuredMesh> shallowCopyToStructuredMesh();

    static LocalIds toLocal(const StructuredBlockGlobalIds& global_ids,
                            const std::map<long, int>& global_to_local);
    const LocalIds local_nodes;
    const LocalIds local_cells;

  private:
    class Block : public StructuredBlock {
      public:
        Block() = default;
        Block(int block_id, StructuredTinfMesh* mesh);
        std::array<int, 3> nodeDimensions() const override;
        Parfait::Point<double> point(int i, int j, int k) const override;
        void setPoint(int i, int j, int k, const Parfait::Point<double>& p) override;
        int blockId() const override;
        int block_id;
        StructuredTinfMesh* self;
    };

    std::map<int, Block> blocks;
};
}
