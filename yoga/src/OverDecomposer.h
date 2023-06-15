#pragma once
#include "MeshSystemInfo.h"
#include "VoxelFragment.h"
namespace YOGA {

class OverDecomposer {
  public:
    static std::vector<VoxelFragment> extractFragments(const YogaMesh& mesh,
                                                       const std::vector<int>& part,
                                                       const std::vector<int>& owned_cell_ids,
                                                       int n_partitions,
                                                       int rank);
    static std::vector<Parfait::Point<double>> generateCellCenters(const YogaMesh& mesh,
        const std::vector<int>& cell_ids);
    static std::vector<std::vector<int>> mapCellsToPartitions(int npart,
                                                              const std::vector<int>& part,
                                                              const std::vector<int>& owned_cell_ids);
    static std::vector<int> identifyOwnedCells(const YogaMesh& mesh,
                                               const PartitionInfo& info,
                                               const MeshSystemInfo& mesh_system_info);

    static int calcNumberOfPartitions(int n_overlap_cells,int target_size){
        if(n_overlap_cells == 0) return 1;
        return n_overlap_cells / target_size + 1;
    }
};
}
