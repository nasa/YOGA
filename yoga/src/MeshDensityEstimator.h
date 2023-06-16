#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/CartBlock.h>
#include "YogaMesh.h"

namespace YOGA {

class MeshDensityEstimator {
  public:
    static std::vector<double> tallyNodesContainedByCartCells(MessagePasser mp,
                                                           const YogaMesh& mesh,
                                                           const std::vector<Parfait::Extent<double>>& component_extents,
                                                           Parfait::CartBlock& block);

  private:
    static std::vector<int> tallyNodesLocally(const YogaMesh& mesh, Parfait::CartBlock& block,
            const std::vector<Parfait::Extent<double>>& component_extents,
            int rank);
    static std::vector<double> tallyCrossingCellsLocally(const YogaMesh& mesh, Parfait::CartBlock& block,
                                              const std::vector<Parfait::Extent<double>>& component_extents,
                                              int rank);
};

}
