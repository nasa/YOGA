#pragma once
#include <MessagePasser/MessagePasser.h>
#include "LoadBalancer.h"
#include "MeshSystemInfo.h"
#include "PartitionInfo.h"
#include "Receptor.h"
#include "ScalableHoleMap.h"
#include "YogaMesh.h"
#include "YogaStatuses.h"

namespace YOGA {

std::vector<ScalableHoleMap> createHoleMaps(MessagePasser mp,
                                            const YogaMesh& view,
                                            const PartitionInfo& partitionInfo,
                                            const MeshSystemInfo& meshSystemInfo,
                                            int max_cells_per_hole_map);
std::vector<Parfait::Extent<double>> getComponentExtents(const MeshSystemInfo& info);
bool doesOverlapOtherComponent(const std::vector<Parfait::Extent<double>>& component_extents,
                               const Parfait::Point<double>& p,
                               int component_of_point);
    bool doesOverlapOtherComponent(const std::vector<Parfait::Extent<double>>& component_extents,
                                   const Parfait::Extent<double>& p,
                                   int component_of_point);
std::vector<Parfait::Point<double>> extractPointsForComponentGrid(
    const YogaMesh& mesh, const std::vector<Parfait::Extent<double>>& component_extents, int id);
std::vector<bool> getInitialWorkUnitMask(MessagePasser mp, std::shared_ptr<LoadBalancer> load_balancer);
std::vector<Parfait::Extent<double>> getInitialWorkUnits(MessagePasser mp, std::shared_ptr<LoadBalancer> load_balancer);
std::vector<Receptor> removeNonReceptors(const std::vector<Receptor>& receptors,
                                         const std::vector<NodeStatus>& node_statuses,
                                         const std::map<long, int>& global_to_local_node_id);

}
