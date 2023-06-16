#include <DcifDistributor.h>
namespace YOGA {
namespace Dcif {
    std::map<int, std::vector<Receptor>> mapReceptorsToRanks(const std::vector<Receptor>& receptors,
                                                                   long global_node_count,
                                                                   int nranks) {
        std::map<int, std::vector<Receptor>> receptors_for_ranks;
        for (auto& receptor : receptors) {
            int target_rank =
                int(Parfait::LinearPartitioner::getWorkerOfWorkItem(receptor.gid, global_node_count, nranks));
            receptors_for_ranks[target_rank].emplace_back(receptor);
        }
        return receptors_for_ranks;
    }
    std::vector<Receptor> getChunkOfReceptors(const Dcif::FlattenedDomainConnectivity& dcif,
                                                    long begin,
                                                    long end) {
        std::vector<Receptor> receptors;
        long n = end - begin;
        for (long i = 0; i < n; i++) receptors.emplace_back(dcif.getReceptor(begin + i));
        return receptors;
    }
    std::vector<Receptor> exchangeReceptors(MessagePasser mp,
                                                  const std::vector<Receptor>& resident_receptors,
                                                  std::map<int, std::vector<long>>& rank_to_requested_receptors) {
        auto global_to_receptor_index = buildGlobalToReceptorIndex(resident_receptors);

        rank_to_requested_receptors = mp.Exchange(rank_to_requested_receptors);
        std::map<int, MessagePasser::Message> replies;
        for (auto& pair : rank_to_requested_receptors) {
            int target_rank = pair.first;
            auto& gids = pair.second;
            long n_receptors_to_send = gids.size();
            auto& msg = replies[target_rank];
            msg.pack(n_receptors_to_send);
            for (long gid : gids) {
                int receptor_index = global_to_receptor_index.at(gid);
                auto r = resident_receptors[receptor_index];
                r.pack(msg);
            }
        }

        replies = mp.Exchange(replies);
        std::vector<Receptor> unpacked_receptors;
        for (auto& pair : replies)
            for (auto receptor : unpackReceptors(pair.second)) unpacked_receptors.emplace_back(receptor);
        return unpacked_receptors;
    }
    std::vector<Receptor> unpackReceptors(MessagePasser::Message& msg) {
        std::vector<Receptor> receptors;
        long n;
        msg.unpack(n);
        for (int i = 0; i < n; i++) {
            Receptor unpacked_receptor;
            msg.unpack(unpacked_receptor.gid);
            msg.unpack(unpacked_receptor.donor_ids);
            msg.unpack(unpacked_receptor.donor_weights);
            receptors.push_back(unpacked_receptor);
        }
        return receptors;
    }
    std::map<long, int> buildGlobalToReceptorIndex(const std::vector<Receptor>& resident_receptors) {
        std::map<long, int> global_to_receptor_index;
        for (int i = 0; i < int(resident_receptors.size()); i++) {
            auto& receptor = resident_receptors[i];
            global_to_receptor_index[receptor.gid] = i;
        }
        return global_to_receptor_index;
    }
    Receptor FlattenedDomainConnectivity::getReceptor(long i) const {
        PARFAIT_ASSERT(i >= 0, "Invalid id: " + std::to_string(i));
        PARFAIT_ASSERT(i < long(fringe_ids.size()), "Invalid id: " + std::to_string(i));
        long gid = fringe_ids[i];
        int ndonors_for_receptor = donor_counts[i];
        long offset = donor_offsets[i];
        std::vector<long> donors(donor_ids.begin() + offset, donor_ids.begin() + offset + ndonors_for_receptor);
        std::vector<double> weights(donor_weights.begin() + offset,
                                    donor_weights.begin() + offset + ndonors_for_receptor);
        return Receptor({gid, donors, weights});
    }
}
}