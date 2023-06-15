#pragma once
#include <parfait/Point.h>
#include "BoundaryConditions.h"
namespace YOGA {

struct TransferNode {
    TransferNode(long global_id,
                 const Parfait::Point<double>& p,
                 double distanceToTheWall,
                 int componentId,
                 int owning_rank)
        : globalId(global_id),
          xyz(p),
          associatedComponentId(componentId),
          owningRank(owning_rank),
          distanceToWall(distanceToTheWall) {}
    TransferNode()
        : globalId(0),
          xyz(0, 0, 0),
          associatedComponentId(0),
          owningRank(0),
          distanceToWall(0) {}
    long globalId;
    Parfait::Point<double> xyz;
    int associatedComponentId;
    int owningRank;
    double distanceToWall;
};
}
