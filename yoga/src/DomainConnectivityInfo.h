#pragma once
#include "YogaStatuses.h"
#include "YogaMesh.h"
#include "ReceptorUpdate.h"
#include "DonorWeightExchanger.h"
#include "InterpolationTools.h"

namespace YOGA{

template<typename T>
    class DomainConnectivityInfo{
      public:
        DomainConnectivityInfo(MessagePasser mp,
                               YogaMesh& mesh,
                               const std::vector<NodeStatus>& statuses,
                               const std::map<int,OversetData::DonorCell>& receptors){
            extractStatusesForOwnedNodes(mp.Rank(),mesh,statuses);
            long nblanked = 0;
            for(auto s:nodeStatuses)
                if(NodeStatus::FringeNode == s) nblanked++;
            nblanked = mp.ParallelSum(nblanked);

            auto inverse_receptors = generateInverseReceptors<T>(mp, receptors, mesh);

            auto weight_calculator = [](const double* xyz, int n,const double* vertices, double* w) {
                LeastSquaresInterpolation<T>::calcWeights(n,(T*)vertices,(T*)xyz,(T*)w);
            };

            // extract donor points
            std::map<int,MessagePasser::Message> donor_points_for_ranks;
            for(auto& pair:inverse_receptors){
                int query_rank = pair.first;
                auto donor_points = extractDonorPointsForInverseReceptors<T>(mesh,pair.second, weight_calculator,mp.Rank());
                auto& msg = donor_points_for_ranks[query_rank];
                msg.pack(int(donor_points.size()));
                for(auto& donor:donor_points)
                    donor.packSelf(msg);
            }

            // exchange back to querying rank
            auto donor_point_msgs_from_ranks = mp.Exchange(donor_points_for_ranks);
            std::map<int,std::vector<DonorPoints<T>>> donor_points_from_ranks;
            for(auto& pair: donor_point_msgs_from_ranks){
                int donor_rank = pair.first;
                auto& msg = pair.second;
                int nquery_points = 0;
                msg.unpack(nquery_points);
                donor_points_from_ranks[donor_rank].reserve(nquery_points);
                for(int i=0;i<nquery_points;i++){
                    donor_points_from_ranks[donor_rank].emplace_back(DonorPoints<T>(msg));
                }
            }

            // turn into dcif data
            for(auto& pair:donor_points_from_ranks){
                auto& donor_points = pair.second;
                for(auto& d:donor_points){
                    int receptor_local_id = d.query_point_id;
                    long gid = mesh.globalNodeId(receptor_local_id);
                    receptorGids.push_back(gid);
                    donorGids.push_back(d.global_ids);
                    donorLocalIds.push_back(d.local_ids);
                    donorOwners.push_back(d.owning_ranks);
                    weights.push_back(d.weights);

                }
            }

            long nreceptor = receptorGids.size();
            nreceptor = mp.ParallelSum(nreceptor);
            if(mp.Rank() == 0)
                printf("Total receptors: %ld\n",nreceptor);
        }
        std::vector<NodeStatus> nodeStatuses;
        std::vector<long> receptorGids;
        std::vector<std::vector<long>> donorGids;
        std::vector<std::vector<int>> donorLocalIds;
        std::vector<std::vector<int>> donorOwners;
        std::vector<std::vector<T>> weights;


      private:
        void extractStatusesForOwnedNodes(int rank, const YogaMesh& mesh, const std::vector<NodeStatus>& statuses){
            if(int(statuses.size()) != mesh.nodeCount()){
                throw std::domain_error("Mesh has "+std::to_string(mesh.nodeCount())+" nodes, but "+std::to_string(statuses.size())+" statuses");
            }
            for(int i=0;i<mesh.nodeCount();i++){
                if(rank == mesh.nodeOwner(i))
                    nodeStatuses.push_back(statuses[i]);
            }
        }

    };


}

