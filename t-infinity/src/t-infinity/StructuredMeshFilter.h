#pragma once
#include <parfait/Throw.h>
#include "StructuredNodeRangeSelector.h"
#include "SMesh.h"

namespace inf {
class StructuredMeshFilter {
  public:
    StructuredMeshFilter(std::shared_ptr<StructuredMesh> mesh,
                         const StructuredNodeRangeSelector& selector)
        : output_mesh(std::make_shared<StructuredSubMesh>(mesh, selector.getRanges(*mesh))) {}
    std::shared_ptr<StructuredSubMesh> getMesh() const { return output_mesh; }
    std::shared_ptr<SField> apply(std::shared_ptr<inf::StructuredField> field) const {
        PARFAIT_ASSERT(field->association() == FieldAttributes::Node(),
                       "StructuredMeshFilter only supports filtering node fields");
        auto filtered = std::make_shared<SField>(field->name(), field->association());
        for (int block_id : output_mesh->blockIds()) {
            const auto& block = output_mesh->getBlock(block_id);
            auto dimensions = block.nodeDimensions();
            auto f = SField::createBlock(dimensions, block_id);
            loopMDRange({0, 0, 0}, dimensions, [&](int i, int j, int k) {
                auto previous = block.previousNodeBlockIJK(i, j, k);
                auto value = field->getBlockField(previous.block_id)
                                 .value(previous.i, previous.j, previous.k);
                f->setValue(i, j, k, value);
            });
            filtered->add(f);
        }
        return filtered;
    }

  private:
    std::shared_ptr<StructuredSubMesh> output_mesh;
};

}
