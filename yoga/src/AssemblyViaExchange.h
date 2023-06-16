#pragma once
#include "OversetData.h"
#include "YogaMesh.h"
#include "VoxelFragment.h"

namespace YOGA {
std::shared_ptr<OversetData> assemblyViaExchange(MessagePasser mp,
                                                 YogaMesh& view,
                                                 int load_balancer_algorithm,
                                                 int target_voxel_size,
                                                 int extra_layers,
                                                 int rcb_agglom_ncells,
                                                 bool should_add_max_receptors,
                                                 const std::vector<int>& component_grid_importance,
                                                 std::function<bool(double*, int, double*)> is_in_cell);



int countRequiredCommRounds(long max_per_round,const std::vector<long>& query_point_counts);

}