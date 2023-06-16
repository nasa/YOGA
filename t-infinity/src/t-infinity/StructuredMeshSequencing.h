#pragma once
#include <memory>
#include "StructuredMesh.h"
#include "StructuredBlockFace.h"

namespace inf {
std::shared_ptr<StructuredBlock> coarsenStructuredBlock(
    const StructuredBlock& block, const std::array<int, 3>& coarsening_factors);
std::shared_ptr<StructuredMesh> coarsenStructuredMesh(const StructuredMesh& mesh,
                                                      const std::array<int, 3>& coarsening_factors);
std::shared_ptr<StructuredBlock> doubleStructuredBlock(const StructuredBlock& mesh, BlockAxis axis);
}
