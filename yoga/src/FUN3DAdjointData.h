#pragma once
#include <DomainConnectivityInfo.h>
#include <YogaMesh.h>

namespace YOGA {
template<typename T>
class FUN3DAdjointData {
  public:
    FUN3DAdjointData() = delete;
    FUN3DAdjointData(MessagePasser mp,
                     YogaMesh& mesh,
                     const std::vector<NodeStatus>& statuses,
                     const std::map<int, OversetData::DonorCell>& receptors)
    :dci(DomainConnectivityInfo<T>(mp,mesh,statuses,receptors)){
        std::map<long,int> g2l;
        for(int i=0;i<mesh.nodeCount();i++)
            g2l[mesh.globalNodeId(i)] = i;
        int nreceptors = dci.receptorGids.size();
        for(int i=0;i<nreceptors;i++) {
            long gid = dci.receptorGids[i];
            int local_id = g2l[gid];
            node_id_to_receptor_index[local_id] = i;
        }
    }

    int donorCount(int node_id){
        if(node_id_to_receptor_index.count(node_id)==0)
            return 0;
        int receptor_index = node_id_to_receptor_index[node_id];
        return int(dci.donorGids[receptor_index].size());
    }

    std::vector<int> donorIndices(int node_id){
        int receptor_index = node_id_to_receptor_index[node_id];
        return dci.donorLocalIds[receptor_index];
    }

    std::vector<int> donorOwners(int node_id){
        int receptor_index = node_id_to_receptor_index[node_id];
        return dci.donorOwners[receptor_index];
    }

    std::vector<T> donorWeights(int node_id){
        int receptor_index = node_id_to_receptor_index[node_id];
        return dci.weights[receptor_index];
    }

  private:
    DomainConnectivityInfo<T> dci;
    std::map<int,int> node_id_to_receptor_index;
};
}