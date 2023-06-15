#pragma once
#include <vector>
#include <parfait/LinearPartitioner.h>
#include <parfait/Throw.h>
#include <MessagePasser/MessagePasser.h>

namespace YOGA{
namespace Dcif {
    struct Receptor {
        long gid;
        std::vector<long> donor_ids;
        std::vector<double> donor_weights;

        void pack(MessagePasser::Message& msg) {
            msg.pack(gid);
            msg.pack(donor_ids);
            msg.pack(donor_weights);
        }
    };

    class FlattenedDomainConnectivity {
      public:
        FlattenedDomainConnectivity(int component_grid_count) : ngrids(component_grid_count) {}
        int componentGridCount() { return ngrids; }
        template<typename T>
        void setReceptorIds(const std::vector<T>& ids) {fringe_ids = std::vector<long>(ids.begin(),ids.end());}
        template<typename T>
        void setDonorCounts(const std::vector<T>& donors_per_receptor) {
            donor_counts = std::vector<int>(donors_per_receptor.begin(),donors_per_receptor.end());
            ndonors = 0;
            for (auto n : donor_counts) ndonors += n;
            donor_offsets.resize(donor_counts.size(), 0);
            for (long i = 1; i < long(donor_offsets.size()); i++)
                donor_offsets[i] = donor_offsets[i - 1] + donor_counts[i - 1];
        }
        template<typename T>
        void setDonorIds(const std::vector<T>& donor_ids_for_receptors) {
            donor_ids = std::vector<long>(donor_ids_for_receptors.begin(),donor_ids_for_receptors.end());
            PARFAIT_ASSERT(ndonors == long(donor_ids.size()), "Donor ids don't match donor counts");
        }
        void setDonorWeights(const std::vector<double>& weights) {
            donor_weights = weights;
            PARFAIT_ASSERT(ndonors == long(donor_weights.size()), "Donor weights don'r match donor counts");
        }
        template<typename T>
        void setIBlank(const std::vector<T>& iblank_for_all_nodes) {
            iblank = std::vector<int>(iblank_for_all_nodes.begin(),iblank_for_all_nodes.end());
        }
        std::vector<long> getFringeIds() { return fringe_ids; }
        std::vector<int> getIblank() { return iblank; }
        std::vector<int> getDonorCounts() { return donor_counts; }
        std::vector<long> getDonorIds() { return donor_ids; }
        std::vector<double> getDonorWeights() { return donor_weights; }

        long receptorCount() const { return fringe_ids.size(); }

        Receptor getReceptor(long i) const;

      private:
        int ngrids;
        long ndonors = 0;
        std::vector<long> fringe_ids;
        std::vector<int> donor_counts;
        std::vector<long> donor_offsets;
        std::vector<long> donor_ids;
        std::vector<double> donor_weights;
        std::vector<int> iblank;
    };

    std::map<long, int> buildGlobalToReceptorIndex(const std::vector<Receptor>& resident_receptors);

    std::vector<Receptor> unpackReceptors(MessagePasser::Message& msg);

    std::vector<Receptor> exchangeReceptors(MessagePasser mp,
                                        const std::vector<Receptor>& resident_receptors,
                                        std::map<int, std::vector<long>>& rank_to_requested_receptors);


    std::vector<Receptor> getChunkOfReceptors(const FlattenedDomainConnectivity& dcif, long begin, long end);

    std::map<int, std::vector<Receptor>> mapReceptorsToRanks(const std::vector<Receptor>& receptors,
                                                                     long global_node_count,
                                                                     int nranks);
}
}