#pragma once
#include <parfait/RecursiveBisection.h>
#include "NC_PartitionerBase.h"

namespace NC {
class RCBPartitioner : public Partitioner {
  public:
    std::vector<int> generatePartVector(MessagePasser mp, const NaiveMesh& nm) const override {
        std::vector<Parfait::Point<double>> points;
        for (size_t local = 0; local < nm.local_to_global_node.size(); local++) {
            auto global = nm.local_to_global_node.at(local);
            if (nm.doOwnNode(global)) points.push_back(nm.xyz[local]);
        }

        return Parfait::recursiveBisection(mp, points, mp.NumberOfProcesses(), 1.0e-4);
    }
};
}
