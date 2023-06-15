#pragma once

#include "MeshSystemInfo.h"
namespace YOGA {

class WorkVoxelBuilder {
  public:

    static std::vector<int> getRanksOfOverlappingPartitions(const MeshSystemInfo& info,const Parfait::Extent<double>& e) {
        std::vector<int> ids;
        for (int i = 0; i < info.numberOfPartitions(); ++i)
            if (info.doesOverlapPartitionExtent(e,i)) ids.push_back(i);
        return ids;
    }

};
}
