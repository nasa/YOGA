#pragma once
#include <MessagePasser/MessagePasser.h>
#include "../plugin-parfait/shared/ParmetisWrapper.h"
#include <vector>
#include "GraphBuilder.h"
#include "NC_PartitionerBase.h"

namespace NC {
class ParmetisPartitioner : public Partitioner {
  public:
    std::vector<int> generatePartVector(MessagePasser mp, const NaiveMesh& nm) const override;
};
inline std::vector<std::vector<long>> buildGraph(const NaiveMesh& mesh) {
    GraphBuilder graphBuilder(mesh);
    return graphBuilder.build();
}

inline std::vector<int> ParmetisPartitioner::generatePartVector(MessagePasser mp, const NaiveMesh& nm) const {
    auto n2n = buildGraph(nm);
    return Parfait::ParmetisWrapper::generatePartVector(mp, n2n);
}
}
