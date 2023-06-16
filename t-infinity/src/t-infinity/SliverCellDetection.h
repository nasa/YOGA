#pragma once
#include <vector>
#include "MeshInterface.h"
#include "Cell.h"

namespace inf {
class SliverCellDetection {
  public:
    /*
     * Calculate the volume ratio of every cell against it's largest neighbor volume.
     * Sliver cells will have volume ratios very tiny.
     * The c2c can be any neighbor adjacency graph including face neighbors
     * where some neighbors may be -1.  Negative neighbors are skipped.
     *
     */
    static std::vector<double> calcWorstCellVolumeRatio(const inf::MeshInterface& mesh,
                                                        const std::vector<std::vector<int>>& c2c);
};
}
