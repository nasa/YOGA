#pragma once
#include <LoadBalancer.h>
#include <set>
#include <queue>
#include <MessagePasser/MessagePasser.h>

namespace YOGA{
    class DistributedLoadBalancer : public LoadBalancer{
      public:
        DistributedLoadBalancer(MessagePasser mp,LoadBalancer& root_balancer,const std::vector<int>& host_ranks){
            std::map<int,std::vector<Parfait::Extent<double>>> voxels_for_ranks;
            if(mp.Rank() == 0){
                while(root_balancer.getRemainingVoxelCount()>0){
                    for(int rank : host_ranks){
                        if(root_balancer.getRemainingVoxelCount()==0) break;
                        voxels_for_ranks[rank].push_back(root_balancer.getWorkVoxel());
                    }
                }
            }
            auto voxels_from_ranks = mp.Exchange(voxels_for_ranks);
            if(std::count(host_ranks.begin(),host_ranks.end(),mp.Rank())){
                auto& v = voxels_from_ranks[0];
                for (auto& e : v) voxels.push(e);
                printf("Load balancer on Rank %i has %li voxels\n",mp.Rank(),voxels.size());
            }
        }
        int getRemainingVoxelCount() override {return voxels.size();}
        Parfait::Extent<double> getWorkVoxel()override {
            if(voxels.empty()) return {};
            auto e = voxels.front();
            voxels.pop();
            return e;
        }

      private:
        std::queue<Parfait::Extent<double>> voxels;
    };
}
