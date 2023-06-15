#include "StructuredMeshSequencing.h"
#include "SMesh.h"
#include "MDVector.h"
#include <parfait/Throw.h>

std::shared_ptr<inf::StructuredBlock> inf::coarsenStructuredBlock(
    const inf::StructuredBlock& block, const std::array<int, 3>& coarsening_factors) {
    auto dimensions = block.nodeDimensions();
    for (int axis = 0; axis < 3; ++axis) {
        if ((dimensions[axis] - 1) % coarsening_factors[axis] != 0) {
            PARFAIT_THROW("invalid coarsening factor: " + std::to_string(coarsening_factors[axis]) +
                          " for axis: " + std::to_string(axis) +
                          " of cell dimension: " + std::to_string(dimensions[axis] - 1));
        }
        dimensions[axis] = (dimensions[axis] - 1) / coarsening_factors[axis] + 1;
    }

    auto coarse_block = SMesh::createBlock(dimensions, block.blockId());
    for (int i = 0; i < dimensions[0]; ++i) {
        for (int j = 0; j < dimensions[1]; ++j) {
            for (int k = 0; k < dimensions[2]; ++k) {
                auto p = block.point(i * coarsening_factors[0],
                                     j * coarsening_factors[1],
                                     k * coarsening_factors[2]);
                coarse_block->points(i, j, k) = p;
            }
        }
    }
    return coarse_block;
}
std::shared_ptr<inf::StructuredMesh> inf::coarsenStructuredMesh(
    const inf::StructuredMesh& mesh, const std::array<int, 3>& coarsening_factors) {
    auto coarse_mesh = std::make_shared<SMesh>();
    for (int block_id : mesh.blockIds()) {
        coarse_mesh->add(coarsenStructuredBlock(mesh.getBlock(block_id), coarsening_factors));
    }
    return coarse_mesh;
}
std::shared_ptr<inf::StructuredBlock> inf::doubleStructuredBlock(const inf::StructuredBlock& block,
                                                                 inf::BlockAxis axis) {
    auto dimensions = block.nodeDimensions();
    dimensions[axis] = 2 * dimensions[axis] - 1;
    auto doubled_block = SMesh::createBlock(dimensions);
    int axis_dimension = block.nodeDimensions()[axis];
    switch (axis) {
        case I: {
            inf::loopMDRange({0, 0, 0}, block.nodeDimensions(), [&](int i, int j, int k) {
                auto p1 = block.point(i, j, k);
                if (i == axis_dimension - 1) {
                    doubled_block->points(2 * i, j, k) = p1;
                } else {
                    auto p2 = block.point(i + 1, j, k);
                    doubled_block->points(2 * i, j, k) = p1;
                    doubled_block->points(2 * i + 1, j, k) = 0.5 * (p1 + p2);
                }
            });
            return doubled_block;
        }
        case J: {
            inf::loopMDRange({0, 0, 0}, block.nodeDimensions(), [&](int i, int j, int k) {
                auto p1 = block.point(i, j, k);
                if (j == axis_dimension - 1) {
                    doubled_block->points(i, 2 * j, k) = p1;
                } else {
                    auto p2 = block.point(i, j + 1, k);
                    doubled_block->points(i, 2 * j, k) = p1;
                    doubled_block->points(i, 2 * j + 1, k) = 0.5 * (p1 + p2);
                }
            });
            return doubled_block;
        }
        case K: {
            inf::loopMDRange({0, 0, 0}, block.nodeDimensions(), [&](int i, int j, int k) {
                auto p1 = block.point(i, j, k);
                if (k == axis_dimension - 1) {
                    doubled_block->points(i, j, 2 * k) = p1;
                } else {
                    auto p2 = block.point(i, j, k + 1);
                    doubled_block->points(i, j, 2 * k) = p1;
                    doubled_block->points(i, j, 2 * k + 1) = 0.5 * (p1 + p2);
                }
            });
            return doubled_block;
        }
    }
    PARFAIT_THROW("Invalid axis: " + std::to_string(axis));
}
