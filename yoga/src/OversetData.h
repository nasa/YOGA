#pragma once
#include "Receptor.h"
#include "YogaStatuses.h"
#include <Tracer.h>

namespace YOGA {

class OversetData {
  public:
    OversetData(std::vector<NodeStatus>&& nodeStatuses,
                const std::vector<Receptor>&& receptorCandidates,
                const std::map<long, int>&& globalToLocal)
        : statuses(nodeStatuses), receptors(createReceptors(receptorCandidates, globalToLocal)) {
        verify();
    }

    const std::vector<NodeStatus> statuses;

    struct DonorCell {
        int id;
        int owningRank;
    };
    const std::map<int, DonorCell> receptors;

  private:
    DonorCell selectBestDonor(const Receptor& r) {
        int idOfBest = 0;
        int smallestDistance = r.candidateDonors[0].distance;
        for (size_t i = 1; i < r.candidateDonors.size(); ++i) {
            if (r.candidateDonors[i].distance < smallestDistance) {
                smallestDistance = r.candidateDonors[i].distance;
                idOfBest = i;
            }
        }
        DonorCell d;
        d.id = r.candidateDonors[idOfBest].cellId;
        d.owningRank = r.candidateDonors[idOfBest].cellOwner;
        return d;
    }

    std::map<int, DonorCell> createReceptors(const std::vector<Receptor>& receptorCandidates,
                                             const std::map<long, int>& globalToLocal) {
        Tracer::begin("create receptor map");
        std::map<int, DonorCell> receptorMap;
        for (auto& r : receptorCandidates) {
            int localId = globalToLocal.at(r.globalId);
            if (FringeNode == statuses[localId]) {
                DonorCell d = selectBestDonor(r);
                receptorMap[localId] = d;
            }
        }
        Tracer::end("create receptor map");
        return receptorMap;
    };

    void verify() {
        for (auto& r : receptors) {
            int id = r.first;
            if (statuses[id] != FringeNode) throw std::logic_error("Receptor has wrong status");
        }
        // for(int i=0;i<statuses.size();++i){
        //	if(receptors.find(i) == receptors.end())
        //		throw std::logic_error("Fringe not in receptor list");
        //}
    }
};

}
