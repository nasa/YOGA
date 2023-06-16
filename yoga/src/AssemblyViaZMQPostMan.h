#pragma once
#include "ZMQPostMan.h"
#include "OversetData.h"
#include "YogaMesh.h"

namespace YOGA {
std::shared_ptr<OversetData> assemblyViaZMQPostMan(MessagePasser mp,
                                                   YogaMesh& view,
                                                   int load_balancer_algorithm,
                                                   int target_voxel_size,
                                                   int extra_layers,
                                                   int rcb_agglom_ncells,
                                                   bool should_add_max_receptors,
                                                   std::function<bool(double*, int, double*)> is_in_cell);
void printBinStats(const std::vector<std::vector<int>>& vecs);
}