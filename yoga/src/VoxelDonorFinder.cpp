#include "VoxelDonorFinder.h"
#include <Tracer.h>

namespace YOGA {

std::vector<std::vector<CandidateDonor>> VoxelDonorFinder::buildCandidateDonorList(
    WorkVoxel& w, const std::vector<bool>& is_node_outside_voxel) {
    Tracer::begin("Build donor finder");
    AdtDonorFinder adtDonorFinder(w);
    Tracer::end("Build donor finder");
    std::vector<std::vector<CandidateDonor>> candidateDonors(w.nodes.size());
    Tracer::begin("query donor finder");
    for (size_t i = 0; i < w.nodes.size(); ++i)
        if (not is_node_outside_voxel[i]) candidateDonors[i] = findCandidateDonors(adtDonorFinder, i, w);
    Tracer::end("query donor finder");
    return candidateDonors;
}

std::vector<CandidateDonor> VoxelDonorFinder::findCandidateDonors(AdtDonorFinder& adtDonorFinder,
                                                                  int i,
                                                                  WorkVoxel& workVoxel) {
    return adtDonorFinder.findDonors(workVoxel.nodes[i].xyz, workVoxel.nodes[i].associatedComponentId);
}

std::vector<bool> VoxelDonorFinder::flagNodesOutsideVoxel(WorkVoxel& w) {
    std::vector<bool> is_outside(w.nodes.size(), false);
    for (size_t i = 0; i < w.nodes.size(); ++i)
        if (not w.extent.intersects(w.nodes[i].xyz)) is_outside[i] = true;
    return is_outside;
}

std::vector<Receptor> VoxelDonorFinder::buildCandidateReceptors(
    WorkVoxel& workVoxel,
    const std::vector<std::vector<CandidateDonor>>& candidateDonors,
    const std::vector<std::set<int>>& n2n) {
    std::vector<Receptor> candidateReceptors;
    for (size_t i = 0; i < candidateDonors.size(); ++i) {
        if (candidateDonors[i].size() > 0) {
            Receptor receptor;
            auto& node = workVoxel.nodes[i];
            receptor.globalId = node.globalId;
            receptor.owner = node.owningRank;
            receptor.distance = node.distanceToWall;
            receptor.candidateDonors = candidateDonors[i];
            for(int nbr:n2n[i]){
                receptor.nbr_gids.push_back(workVoxel.nodes[nbr].globalId);
            }
            candidateReceptors.push_back(receptor);
        }
    }
    return candidateReceptors;
}
std::vector<Receptor> VoxelDonorFinder::getCandidateReceptors(WorkVoxel& workVoxel,
                                                              const Parfait::Extent<double>& extent,
                                                              const std::vector<std::set<int>>& n2n) {
    auto is_node_outside_voxel = flagNodesOutsideVoxel(workVoxel);
    auto candidate_donors = buildCandidateDonorList(workVoxel,is_node_outside_voxel);
    return buildCandidateReceptors(workVoxel,candidate_donors,n2n);
}

}
