
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
#pragma once
#include <vector>
#include <map>
#include <set>
#include <MessagePasser/MessagePasser.h>

namespace Parfait {

class SyncPattern {
  public:
    inline SyncPattern(const std::map<int, std::vector<long>>& send_to,
                       const std::map<int, std::vector<long>>& receive_from)
        : send_to(send_to), receive_from(receive_from) {}

    inline SyncPattern(MessagePasser mp, const std::vector<long>& have_in, const std::vector<long>& need) {
        send_to = getSendTo(mp, have_in, need);
        receive_from = getReceiveFrom(mp, send_to);
    }

    inline SyncPattern(MessagePasser mp, const std::vector<long>& gids, const std::vector<int>& owners) {
        std::vector<long> need, have;
        std::vector<int> need_from;
        for (int i = 0; i < int(gids.size()); i++) {
            auto gid = gids[i];
            auto owner = owners[i];
            if (mp.Rank() == owner) {
                have.push_back(gid);
            } else {
                need.push_back(gid);
                need_from.push_back(owner);
            }
        }
        receive_from = getReceiveFrom(mp, need, need_from);
        send_to = getSendTo(mp, receive_from);
    }

    inline SyncPattern(MessagePasser mp,
                       const std::vector<long>& have_in,
                       const std::vector<long>& need,
                       const std::vector<int>& need_from) {
        receive_from = getReceiveFrom(mp, need, need_from);
        send_to = getSendTo(mp, receive_from);
    }

    static inline std::map<int, std::vector<long>> getSendTo(MessagePasser mp,
                                                             const std::map<int, std::vector<long>>& recv_from) {
        int nrecvs = getNumberOfRanksThatWillSendMeStuff(mp, recv_from);

        std::vector<int> sending_ranks = getIdsOfRanksThatWillSendMeStuff(mp, recv_from, nrecvs);

        std::vector<int> recv_counts = getRecvCounts(mp, recv_from, nrecvs, sending_ranks);

        auto send_statuses = sendGidsNonBlocking(mp, recv_from);

        std::map<int, std::vector<long>> send_to;
        for (int i = 0; i < nrecvs; i++) {
            int rank = sending_ranks[i];
            send_to[rank] = {};
            int n = recv_counts[i];
            mp.Recv(send_to[rank], n, rank);
        }

        mp.WaitAll(send_statuses);
        mp.Barrier();

        return send_to;
    }

    static std::vector<MessagePasser::MessageStatus> sendGidsNonBlocking(
        const MessagePasser& mp, const std::map<int, std::vector<long>>& recv_from) {
        std::vector<MessagePasser::MessageStatus> send_statuses;
        for (auto& pair : recv_from) {
            int target_rank = pair.first;
            auto& gids = pair.second;
            send_statuses.push_back(mp.NonBlockingSend(gids, target_rank));
        }
        return send_statuses;
    }
    static std::vector<int> getRecvCounts(const MessagePasser& mp,
                                          const std::map<int, std::vector<long>>& recv_from,
                                          int nrecvs,
                                          const std::vector<int>& sending_ranks) {
        std::vector<MessagePasser::MessageStatus> send_statuses;
        std::map<int, int> gid_sizes;
        for (auto& pair : recv_from) {
            int target_rank = pair.first;
            auto& gids = pair.second;
            gid_sizes[target_rank] = gids.size();
        }

        for (auto& pair : recv_from) {
            int target_rank = pair.first;
            send_statuses.push_back(mp.NonBlockingSend(gid_sizes.at(target_rank), target_rank));
        }
        std::vector<int> recv_counts(nrecvs);
        for (int i = 0; i < nrecvs; i++) {
            int rank = sending_ranks[i];
            mp.Recv(recv_counts[i], rank);
        }
        mp.WaitAll(send_statuses);
        mp.Barrier();
        return recv_counts;
    }
    static std::vector<int> getIdsOfRanksThatWillSendMeStuff(const MessagePasser& mp,
                                                             const std::map<int, std::vector<long>>& recv_from,
                                                             int nrecvs) {
        std::vector<MessagePasser::MessageStatus> send_statuses;
        int rank = mp.Rank();
        for (auto& pair : recv_from) {
            int target_rank = pair.first;
            send_statuses.push_back(mp.NonBlockingSend(rank, target_rank));
        }

        std::vector<int> sending_ranks(nrecvs, -9);
        for (int i = 0; i < nrecvs; i++) {
            mp.Recv(sending_ranks[i]);
        }
        mp.WaitAll(send_statuses);
        mp.Barrier();
        return sending_ranks;
    }
    static int getNumberOfRanksThatWillSendMeStuff(const MessagePasser& mp,
                                                   const std::map<int, std::vector<long>>& recv_from) {
        std::vector<int> number_of_recvs_to_post_for_each_rank(mp.NumberOfProcesses(), 0);
        for (auto& pair : recv_from) {
            int rank = pair.first;
            number_of_recvs_to_post_for_each_rank[rank] = 1;
        }
        mp.ElementalSum(number_of_recvs_to_post_for_each_rank, 0);
        int nrecvs = mp.Scatter(number_of_recvs_to_post_for_each_rank, 0);
        return nrecvs;
    }

    static std::vector<int> determineHowManyISendToEachRank(MessagePasser mp, const std::vector<int>& need_from) {
        std::vector<int> i_need_this_many_from_you(mp.NumberOfProcesses(), 0);
        for (int from : need_from) {
            i_need_this_many_from_you[from]++;
        }

        std::vector<int> i_will_send_you_this_many(mp.NumberOfProcesses(), 0);
        std::vector<MessagePasser::MessageStatus> statuses;
        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            statuses.emplace_back(mp.NonBlockingSend(i_need_this_many_from_you[r], r));
            statuses.emplace_back(mp.NonBlockingRecv(i_will_send_you_this_many[r], r));
        }

        mp.WaitAll(statuses);
        return i_will_send_you_this_many;
    }

    static inline std::map<int, std::vector<long>> getSendTo(MessagePasser mp,
                                                             const std::vector<long>& have_in,
                                                             std::vector<long> need) {
        std::set<long> have(have_in.begin(), have_in.end());
        std::map<int, std::set<long>> send_set;
        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            std::vector<long> current_need;
            if (r == mp.Rank()) current_need = need;
            mp.Broadcast(current_need, r);
            for (auto thing : current_need) {
                if (have.count(thing) != 0) {
                    send_set[r].insert(thing);
                };
            }
        }
        std::map<int, std::vector<long>> send_to;
        for (auto& pair : send_set) {
            auto& target_rank = pair.first;
            auto& data = pair.second;
            send_to[target_rank] = std::vector<long>(data.begin(), data.end());
        }
        return send_to;
    }

    static inline std::map<int, std::vector<long>> getReceiveFrom(MessagePasser mp,
                                                                  const std::vector<long>& need,
                                                                  const std::vector<int>& need_from) {
        std::map<int, std::vector<long>> recv_from;
        for (size_t i = 0; i < need.size(); i++) {
            long global_id = need[i];
            int from_rank = need_from[i];
            recv_from[from_rank].push_back(global_id);
        }
        return recv_from;
    }

    static inline std::map<int, std::vector<long>> getReceiveFrom(MessagePasser mp,
                                                                  const std::map<int, std::vector<long>>& send_to) {
        std::vector<long> receive_count(mp.NumberOfProcesses());
        std::vector<MPI_Request> requests(mp.NumberOfProcesses());
        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            MPI_Irecv(&receive_count[r], sizeof(long), MPI_CHAR, r, 0, mp.getCommunicator(), &requests[r]);
        }

        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            long count = 0;
            if (send_to.count(r)) count = send_to.at(r).size();
            mp.Send(count, r);
        }

        std::map<int, std::vector<long>> receive_from;
        std::vector<MessagePasser::MessageStatus> statuses;
        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            MPI_Status s;
            MPI_Wait(&requests[r], &s);
        }

        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            long count = receive_count[r];
            if (count != 0) {
                receive_from[r].resize(receive_count[r]);
                auto status = mp.NonBlockingRecv(receive_from[r], receive_from[r].size(), r);
                statuses.push_back(status);
            }
        }

        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            if (send_to.count(r)) {
                mp.Send(send_to.at(r), r);
            }
        }

        mp.WaitAll(statuses);

        return receive_from;
    };

    std::map<int, std::vector<long>> send_to;
    std::map<int, std::vector<long>> receive_from;
};

}
