#pragma once
#include <MessagePasser/MessagePasser.h>
#include "YogaMesh.h"
#include "InverseReceptor.h"
#include <iostream>
#include <parfait/ExtentBuilder.h>
#include <ddata/ComplexDifferentiation.h>

namespace YOGA{

template<typename T>
class DonorPoints{
  public:
    template <typename WeightCalculator>
    DonorPoints(const YogaMesh& mesh, const InverseReceptor<T>& ir,WeightCalculator calc_weights,int rank){
        query_point_id = ir.local_node_id;
        int cell_id = ir.donor_cell_id;
        int n = mesh.numberOfNodesInCell(cell_id);
        local_ids.resize(n);
        global_ids.resize(n);
        locations.resize(n);
        owning_ranks.resize(n);
        weights.resize(n);
        mesh.getNodesInCell(cell_id,local_ids.data());
        for(int i=0;i<n;i++) {
            int id = local_ids[i];
            global_ids[i] = mesh.globalNodeId(id);
            locations[i] = mesh.getNode<T>(id);
            //owning_ranks[i] = mesh.nodeOwner(id);
            // FUN3D is going to do its own communication based on local id, so claim to own, even if ghost
            // so that the local id will be correct.
            owning_ranks[i] = rank;
        }
        calc_weights((double*)ir.p.data(), n, (double*)locations.front().data(),(double*)weights.data());
        verifyWeightsRecoveryQueryPoint(ir.p);
    }

    void packSelf(MessagePasser::Message& msg){
        msg.pack(query_point_id);
        msg.pack(local_ids);
        msg.pack(global_ids);
        msg.pack(owning_ranks);
        msg.pack(locations);
        msg.pack(weights);
    }

    DonorPoints(MessagePasser::Message& msg){
        msg.unpack(query_point_id);
        msg.unpack(local_ids);
        msg.unpack(global_ids);
        msg.unpack(owning_ranks);
        msg.unpack(locations);
        msg.unpack(weights);
    }

    static DonorPoints unpack(MessagePasser::Message& msg){
        return DonorPoints(msg);
    }

    int query_point_id;
    std::vector<int> local_ids;
    std::vector<long> global_ids;
    std::vector<int> owning_ranks;
    std::vector<Parfait::Point<T>> locations;
    std::vector<T> weights;

  private:
    void verifyWeightsRecoveryQueryPoint(const Parfait::Point<T>& p){
        using namespace std;
        using namespace Linearize;
        Parfait::Point<T> interpolated_xyz {0,0,0};
        auto donor_extent = Parfait::ExtentBuilder::createEmptyBuildableExtent<T>();
        for(int i=0;i<int(weights.size());i++){
            for(int j=0; j<3;j++) {
                interpolated_xyz[j] += weights[i] * locations[i][j];
                donor_extent.lo[j] = min(locations[i][j], donor_extent.lo[j]);
                donor_extent.hi[j] = max(locations[i][j], donor_extent.hi[j]);
            }

        }
        T biggest_distance_from_origin = max(donor_extent.hi.magnitude(),donor_extent.lo.magnitude());
        T donor_extent_size = (donor_extent.hi - donor_extent.lo).magnitude();
        T scale = max(biggest_distance_from_origin, donor_extent_size);
        scale = max(1.0, scale);
        T tol = 1.0e-6 * scale;
        if(real(interpolated_xyz.distance(p)) > real(tol)){
            std::stringstream ss;
            ss << "Weights do not recover receptor point (error: " <<
                interpolated_xyz.distance(p) << ", tol: " << tol << ")\n";
            for(auto w:weights)
                ss << "weights.push_back("<<w <<");\n";
            ss << "\nDonor points:\n";
            for(auto xyz:locations)
                ss << "points.push_back({" << xyz[0] << ", " << xyz[1] << ", " << xyz[2] << "});\n";
            ss << "\nProjected point: " << interpolated_xyz[0] << ", " << interpolated_xyz[1] << ", " << interpolated_xyz[2];
            ss << "\nreceptor = {" << p[0] << ", " << p[1] << ", " << p[2] << "};\n";
            throw std::logic_error(ss.str());
        }
    }
};

template<typename T, typename WeightCalculator>
std::vector<DonorPoints<T>> extractDonorPointsForInverseReceptors(const YogaMesh& mesh,
                                                               const std::vector<InverseReceptor<T>>& inverse_receptors,
                                                               WeightCalculator calc_weights,
                                                               int rank){
    std::vector<DonorPoints<T>> donors;
    donors.reserve(inverse_receptors.size());
    for(auto& ir:inverse_receptors)
        donors.emplace_back(DonorPoints<T>(mesh,ir,calc_weights,rank));
    return donors;
}


}