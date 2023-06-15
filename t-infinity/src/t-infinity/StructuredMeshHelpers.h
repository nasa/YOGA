#pragma once
#include <MessagePasser/MessagePasser.h>
#include "StructuredMesh.h"
#include "SMesh.h"

namespace inf {
std::map<int, int> buildBlockToRank(const MessagePasser& mp, const std::vector<int>& block_ids_on_rank);
bool is2D(const MessagePasser& mp, const StructuredMesh& mesh);
std::map<int, std::array<int, 3>> buildAllBlockDimensions(const MessagePasser& mp, const StructuredMesh& mesh);

std::shared_ptr<StructuredBlockField> makeConstantValueField(
    const inf::StructuredMesh& mesh,
    int block_id,
    double value = 0.0,
    std::string association = inf::FieldAttributes::Unassigned());
std::shared_ptr<StructuredField> makeConstantValueField(const inf::StructuredMesh& mesh,
                                                        std::string name,
                                                        double value = 0.0,
                                                        std::string association = inf::FieldAttributes::Unassigned());
}
