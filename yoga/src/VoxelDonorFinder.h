#pragma once
#include "AdtDonorFinder.h"
#include "Receptor.h"
#include "ScalableHoleMap.h"
#include "WorkVoxel.h"

namespace YOGA {
class VoxelDonorFinder {
  public:
    static std::vector<Receptor> getCandidateReceptors(
        WorkVoxel& workVoxel,
        const Parfait::Extent<double>& extent,
        const std::vector<std::set<int>>& n2n);
    //1static std::vector<int> getLocalIdsOfInterpolationBoundaryNodes(std::vector<TransferNode>& nodes);
    static std::vector<bool> flagNodesOutsideVoxel(WorkVoxel& w);

  private:
    static std::vector<CandidateDonor> findCandidateDonors(AdtDonorFinder& adtDonorFinder, int i, WorkVoxel& workVoxel);
    static std::vector<std::vector<CandidateDonor>> buildCandidateDonorList(
        WorkVoxel& w, const std::vector<bool>& is_node_outside_voxel);
    static std::vector<Receptor> buildCandidateReceptors(
        WorkVoxel& workVoxel,
        const std::vector<std::vector<CandidateDonor>>& candidateDonors,
        const std::vector<std::set<int>>& n2n);
};

}
