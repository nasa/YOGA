#pragma once
#include <vector>
#include <parfait/Point.h>
#include "YogaMesh.h"
#include <parfait/Inspector.h>
#include "PartitionInfo.h"
#include "VoxelFragment.h"
#include "MeshSystemInfo.h"

namespace YOGA {

typedef std::map<int, VoxelFragment> FragmentMap;
typedef std::map<int, std::vector<bool>> AffinityMap;

std::pair<FragmentMap, AffinityMap> createAndBalanceFragments(MessagePasser mp,
                                                              const YogaMesh& mesh,
                                                              const PartitionInfo& partition_info,
                                                              const MeshSystemInfo& mesh_system_info,
                                                              const std::map<long, int>& g2l,
                                                              int rcb_agglom_ncells,
                                                              Parfait::Inspector& inspector);


std::map<int, std::vector<bool>> buildNodeAffinities(const YogaMesh& view,
                                                     const std::map<long, int>& g2l,
                                                     const std::map<int, VoxelFragment>& fragments_for_ranks);
struct Agglomeration {
    std::vector<Parfait::Point<double>> points;
    std::vector<std::vector<int>> ids;
};
Agglomeration agglomerateCells(const YogaMesh& view,
                               Parfait::Inspector& inspector,
                               const PartitionInfo& partition_info,
                               const MeshSystemInfo& mesh_system_info,
                               int rcb_agglom_ncells);

std::map<int, std::vector<bool>> exchangeNodeAffinities(const MessagePasser& mp,
                                                        const std::map<int, std::vector<bool>>& node_fragment_affinity);

std::map<int, VoxelFragment> extractFragmentsForRanks(const MessagePasser& mp,
                                                      const YogaMesh& view,
                                                      const std::map<int, std::vector<int>>& cell_ids_for_partitions);

std::map<int, std::vector<int>> mapCellIdsToRanks(const std::vector<std::vector<int>>& agglomerated_ids,
                                                  const std::vector<int>& part);


}
