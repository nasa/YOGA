#include "StructuredTinfMesh.h"
#include "SMesh.h"

inf::StructuredTinfMesh::StructuredTinfMesh(const inf::TinfMeshData& data,
                                            int partition_id,
                                            LocalIds local_nodes,
                                            LocalIds local_cells)
    : TinfMesh(data, partition_id),
      local_nodes(std::move(local_nodes)),
      local_cells(std::move(local_cells)) {
    for (int block_id : blockIds()) blocks[block_id] = Block(block_id, this);
}

inf::StructuredTinfMesh::StructuredTinfMesh(const inf::TinfMeshData& data,
                                            int partition_id,
                                            const inf::StructuredBlockGlobalIds& global_nodes,
                                            const inf::StructuredBlockGlobalIds& global_cells)
    : TinfMesh(data, partition_id),
      local_nodes(toLocal(global_nodes, GlobalToLocal::buildNode(*this))),
      local_cells(toLocal(global_cells, GlobalToLocal::buildCell(*this))) {
    for (int block_id : blockIds()) blocks[block_id] = Block(block_id, this);
}

inf::StructuredTinfMesh::StructuredTinfMesh(const inf::TinfMesh& mesh,
                                            const inf::StructuredBlockGlobalIds& global_nodes,
                                            const inf::StructuredBlockGlobalIds& global_cells)
    : StructuredTinfMesh(mesh.mesh, mesh.partitionId(), global_nodes, global_cells) {}

int inf::StructuredTinfMesh::cellId(int block_id, int i, int j, int k) const {
    return local_cells.at(block_id).at(i, j, k);
}
int inf::StructuredTinfMesh::nodeId(int block_id, int i, int j, int k) const {
    return local_nodes.at(block_id).at(i, j, k);
}
std::vector<int> inf::StructuredTinfMesh::blockIds() const {
    std::vector<int> block_ids;
    for (const auto& p : local_nodes) block_ids.push_back(p.first);
    return block_ids;
}
inf::StructuredTinfMesh::LocalIds inf::StructuredTinfMesh::toLocal(
    const inf::StructuredBlockGlobalIds& global_ids, const std::map<long, int>& global_to_local) {
    LocalIds local_ids;
    for (const auto& p : global_ids) {
        local_ids[p.first] = Vector3D<int>(p.second.dimensions());
        loopMDRange({0, 0, 0}, p.second.dimensions(), [&](int i, int j, int k) {
            local_ids[p.first](i, j, k) = global_to_local.at(p.second(i, j, k));
        });
    }
    return local_ids;
}

inf::StructuredTinfMesh::Block::Block(int block_id, inf::StructuredTinfMesh* mesh)
    : block_id(block_id), self(mesh) {}
inf::StructuredBlock& inf::StructuredTinfMesh::getBlock(int block_id) {
    return blocks.at(block_id);
}
const inf::StructuredBlock& inf::StructuredTinfMesh::getBlock(int block_id) const {
    return blocks.at(block_id);
}
std::shared_ptr<inf::StructuredBlockField> inf::StructuredTinfMesh::getField(
    int block_id, const inf::FieldInterface& field) const {
    PARFAIT_ASSERT(field.blockSize() == 1, "Only scalar fields are supported");
    PARFAIT_ASSERT(field.attribute(FieldAttributes::DataType()) == FieldAttributes::Float64(),
                   "Only Float64 type fields are supported");
    std::shared_ptr<SField::Block> block_field;
    if (field.association() == FieldAttributes::Node()) {
        block_field = SField::createBlock(local_nodes.at(block_id).dimensions(), block_id);
        loopMDRange({0, 0, 0}, block_field->dimensions(), [&](int i, int j, int k) {
            int node_id = nodeId(block_id, i, j, k);
            field.value(node_id, &block_field->values(i, j, k));
        });
    } else {
        block_field = SField::createBlock(local_cells.at(block_id).dimensions(), block_id);
        loopMDRange({0, 0, 0}, block_field->dimensions(), [&](int i, int j, int k) {
            int cell_id = cellId(block_id, i, j, k);
            field.value(cell_id, &block_field->values(i, j, k));
        });
    }
    return block_field;
}
std::shared_ptr<inf::StructuredField> inf::StructuredTinfMesh::getField(
    const inf::FieldInterface& field) const {
    auto sfield = std::make_shared<inf::SField>(field.name(), field.association());
    for (int block_id : blockIds()) {
        sfield->add(getField(block_id, field));
    }
    return sfield;
}
std::array<int, 3> inf::StructuredTinfMesh::Block::nodeDimensions() const {
    return self->local_nodes.at(block_id).dimensions();
}
Parfait::Point<double> inf::StructuredTinfMesh::Block::point(int i, int j, int k) const {
    int node_id = self->local_nodes.at(block_id)(i, j, k);
    return self->mesh.points[node_id];
}
void inf::StructuredTinfMesh::Block::setPoint(int i,
                                              int j,
                                              int k,
                                              const Parfait::Point<double>& p) {
    int node_id = self->local_nodes.at(block_id)(i, j, k);
    self->mesh.points[node_id] = p;
}
int inf::StructuredTinfMesh::Block::blockId() const { return block_id; }

class ConstStructuredTinfMeshWrapper : public inf::StructuredMesh {
  public:
    ConstStructuredTinfMeshWrapper(const inf::StructuredTinfMesh& mesh) : mesh(mesh) {}
    std::vector<int> blockIds() const override { return mesh.blockIds(); }
    inf::StructuredBlock& getBlock(int block_id) override {
        PARFAIT_THROW("Structured mesh instance is immutable");
    }
    const inf::StructuredBlock& getBlock(int block_id) const override {
        return mesh.getBlock(block_id);
    }
    const inf::StructuredTinfMesh& mesh;
};
class StructuredTinfMeshWrapper : public inf::StructuredMesh {
  public:
    StructuredTinfMeshWrapper(inf::StructuredTinfMesh& mesh) : mesh(mesh) {}
    std::vector<int> blockIds() const override { return mesh.blockIds(); }
    inf::StructuredBlock& getBlock(int block_id) override { return mesh.getBlock(block_id); }
    const inf::StructuredBlock& getBlock(int block_id) const override {
        return mesh.getBlock(block_id);
    }
    inf::StructuredTinfMesh& mesh;
};

std::shared_ptr<inf::StructuredMesh> inf::StructuredTinfMesh::shallowCopyToStructuredMesh() const {
    return std::make_shared<ConstStructuredTinfMeshWrapper>(*this);
}
std::shared_ptr<inf::StructuredMesh> inf::StructuredTinfMesh::shallowCopyToStructuredMesh() {
    return std::make_shared<StructuredTinfMeshWrapper>(*this);
}