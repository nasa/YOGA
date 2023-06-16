#include "StructuredMeshHelpers.h"
#include "MDVector.h"
#include <parfait/Throw.h>
#include <algorithm>

std::map<int, int> inf::buildBlockToRank(const MessagePasser& mp,
                                         const std::vector<int>& block_ids_on_rank) {
    auto rank_to_blocks = mp.Gather(block_ids_on_rank);
    std::map<int, int> block_to_rank_mapping;
    for (int rank = 0; rank < mp.NumberOfProcesses(); ++rank) {
        for (int block_id : rank_to_blocks.at(rank)) {
            PARFAIT_ASSERT(block_to_rank_mapping.count(block_id) == 0,
                           "Structured block ownership is not unique.");
            block_to_rank_mapping[block_id] = rank;
        }
    }
    return block_to_rank_mapping;
}
bool inf::is2D(const MessagePasser& mp, const inf::StructuredMesh& mesh) {
    bool is_2d = true;
    for (int block_id : mesh.blockIds()) {
        auto node_dimensions = mesh.getBlock(block_id).nodeDimensions();
        auto block_is_3d = std::count(node_dimensions.begin(), node_dimensions.end(), 1) == 0;
        if (block_is_3d) {
            is_2d = false;
            break;
        }
    }
    return mp.ParallelAnd(is_2d);
}
std::map<int, std::array<int, 3>> inf::buildAllBlockDimensions(const MessagePasser& mp,
                                                               const inf::StructuredMesh& mesh) {
    std::vector<std::array<int, 3>> dimensions_on_rank;
    auto all_block_ids = mp.Gather(mesh.blockIds(), 0);
    for (int block_id : mesh.blockIds())
        dimensions_on_rank.push_back(mesh.getBlock(block_id).nodeDimensions());

    auto dimensions_per_rank = mp.Gather(dimensions_on_rank, 0);

    std::map<int, std::array<int, 3>> block_dimensions;
    if (mp.Rank() == 0) {
        for (int rank = 0; rank < mp.NumberOfProcesses(); ++rank) {
            for (size_t i = 0; i < all_block_ids.at(rank).size(); ++i) {
                int block_id = all_block_ids[rank][i];
                auto dimensions = dimensions_per_rank[rank][i];
                block_dimensions[block_id] = dimensions;
            }
        }
    }
    mp.Broadcast(block_dimensions, 0);
    return block_dimensions;
}
std::shared_ptr<inf::StructuredBlockField> inf::makeConstantValueField(const inf::StructuredMesh& mesh, int block_id, double value, std::string association) {
    std::shared_ptr<SField::Block> block_field;
    auto& mesh_block = mesh.getBlock(block_id);
    if (association == FieldAttributes::Node()) {
        block_field = SField::createBlock(mesh_block.nodeDimensions(), block_id);
        loopMDRange({0, 0, 0}, block_field->dimensions(), [&](int i, int j, int k) {
            block_field->values(i, j, k) = value;
        });
    } else {
        block_field = SField::createBlock(mesh_block.cellDimensions(), block_id);
        loopMDRange({0, 0, 0}, block_field->dimensions(), [&](int i, int j, int k) {
            block_field->values(i, j, k) = value;
        });
    }
    return block_field;
}
std::shared_ptr<inf::StructuredField> inf::makeConstantValueField(const inf::StructuredMesh& mesh, std::string name, double value, std::string association) {
    auto sfield = std::make_shared<inf::SField>(name, association);
    for (int block_id : mesh.blockIds()) {
        sfield->add(inf::makeConstantValueField(mesh, block_id, value, association));
    }
    return sfield;
}
