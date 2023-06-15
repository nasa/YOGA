#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/SyncField.h>
#include "StructuredMeshGlobalIds.h"
#include "StructuredMeshValues.h"

namespace inf {
using BlockIJK = std::tuple<int, int, int, int, bool>;
using GlobalToBlockIJK = std::map<long, std::vector<BlockIJK>>;

template <typename T>
class ValueWrapper {
  public:
    ValueWrapper(StructuredMeshValues<T>& values, const GlobalToBlockIJK& global_to_block_ijk)
        : data(values),
          source_global_to_block_ijk(global_to_block_ijk),
          target_global_to_block_ijk(global_to_block_ijk) {}
    ValueWrapper(StructuredMeshValues<T>& values,
                 const GlobalToBlockIJK& source,
                 const GlobalToBlockIJK& target)
        : data(values), source_global_to_block_ijk(source), target_global_to_block_ijk(target) {}
    using value_type = T;

    T getEntry(long global_id, int var) {
        T p{};
        if (source_global_to_block_ijk.count(global_id) == 0) {
            MessagePasser mp(MPI_COMM_WORLD);
            printf("[%d] (get) missing gid: %ld\n", mp.Rank(), global_id);
        }
        for (const auto& local : source_global_to_block_ijk.at(global_id)) {
            if (not std::get<4>(local)) continue;
            const int& block_id = std::get<0>(local);
            const int& i = std::get<1>(local);
            const int& j = std::get<2>(local);
            const int& k = std::get<3>(local);
            p = data.getValue(block_id, i, j, k, var);
        }
        return p;
    }
    void setEntry(long global_id, int var, const T& value) {
        if (target_global_to_block_ijk.count(global_id) == 0) {
            MessagePasser mp(MPI_COMM_WORLD);
            printf("[%d] (set) missing gid: %ld\n", mp.Rank(), global_id);
        }
        for (const auto& local : target_global_to_block_ijk.at(global_id)) {
            const int& block_id = std::get<0>(local);
            const int& i = std::get<1>(local);
            const int& j = std::get<2>(local);
            const int& k = std::get<3>(local);
            data.setValue(block_id, i, j, k, var, value);
        }
    }

    int stride() const { return data.blockSize(); }

    StructuredMeshValues<T>& data;
    const GlobalToBlockIJK& source_global_to_block_ijk;
    const GlobalToBlockIJK& target_global_to_block_ijk;
};

class StructuredMeshSyncer {
  public:
    StructuredMeshSyncer(const MessagePasser& mp_in, const StructuredMeshGlobalIds& global_ids)
        : mp(mp_in),
          sync_pattern(buildSyncPattern(mp, global_ids)),
          global_to_block_ijk(buildGlobalToBlockIJK(mp, global_ids)),
          on_rank_block_to_block(buildOnRankSync(global_ids.owner)) {}

    template <typename T>
    void sync(ValueWrapper<T>& values) const {
        Parfait::StridedFieldSyncer<T, ValueWrapper<T>> my_syncer(mp, values, sync_pattern);
        my_syncer.start();
        syncLocal(values.data);
        my_syncer.finish();
    }

    template <typename T>
    void sync(StructuredMeshValues<T>& values) const {
        auto wrapper = ValueWrapper<T>(values, global_to_block_ijk);
        sync(wrapper);
    }

    template <typename T>
    void syncLocal(StructuredMeshValues<T>& values) const {
        for (const auto& p : on_rank_block_to_block) {
            const auto& to = p.first;
            const auto& from = p.second;
            for (int var = 0; var < values.blockSize(); ++var) {
                T v = values.getValue(std::get<0>(from),
                                      std::get<1>(from),
                                      std::get<2>(from),
                                      std::get<3>(from),
                                      var);
                values.setValue(
                    std::get<0>(to), std::get<1>(to), std::get<2>(to), std::get<3>(to), var, v);
            }
        }
    }

    const MessagePasser mp;
    const Parfait::SyncPattern sync_pattern;
    const GlobalToBlockIJK global_to_block_ijk;
    const std::map<BlockIJK, BlockIJK> on_rank_block_to_block;

    static GlobalToBlockIJK buildGlobalToBlockIJK(const MessagePasser& mp,
                                                  const StructuredMeshGlobalIds& global_ids) {
        GlobalToBlockIJK global_to_bijk;
        for (const auto& p : global_ids.ids) {
            int block_id = p.first;
            const auto& block_gids = p.second;
            loopMDRange({0, 0, 0}, block_gids.dimensions(), [&](int i, int j, int k) {
                long gid = block_gids(i, j, k);
                // Skip negative global ids (i.e. for boundary condition cells)
                if (gid >= 0) {
                    bool owned = global_ids.owner.getOwningRank(gid) == mp.Rank();
                    global_to_bijk[gid].push_back({block_id, i, j, k, owned});
                }
            });
        }
        return global_to_bijk;
    }
    static Parfait::SyncPattern buildSyncPattern(const MessagePasser& mp,
                                                 const StructuredMeshGlobalIds& global_ids) {
        std::vector<long> have, need;
        std::vector<int> need_from;
        for (const auto& p : global_ids.ids) {
            const auto& gids = p.second;
            std::array<int, 3> start = {0, 0, 0};
            std::array<int, 3> end = gids.dimensions();
            loopMDRange(start, end, [&](int i, int j, int k) {
                long gid = gids(i, j, k);
                if (gid >= 0) {
                    int owner = global_ids.owner.getOwningRank(gid);
                    if (mp.Rank() == owner) {
                        have.push_back(gid);
                    } else {
                        need.push_back(gid);
                        need_from.push_back(owner);
                    }
                }
            });
        }
        return {mp, have, need, need_from};
    }

  private:
    std::map<BlockIJK, BlockIJK> buildOnRankSync(
        const StructuredMeshGlobalIdOwner& node_owner) const {
        std::map<BlockIJK, BlockIJK> local_sync;
        for (const auto& p : global_to_block_ijk) {
            // Skip nodes that are not shared between blocks
            if (p.second.size() == 1) continue;

            long global = p.first;
            int owning_block = node_owner.getOwningBlock(global);

            // Only sync global ids that shared between on-rank blocks
            if (node_owner.getOwningRank(global) != mp.Rank()) continue;

            auto getOwnerBIJK = [&]() {
                for (const auto& bijk : p.second) {
                    if (std::get<0>(bijk) == owning_block) return bijk;
                }
                for (const auto& bijk : p.second) {
                    printf("[%d] (%d,%d,%d)\n",
                           std::get<0>(bijk),
                           std::get<1>(bijk),
                           std::get<2>(bijk),
                           std::get<3>(bijk));
                }
                printf("global id: %lu block_owner: %d\n", global, owning_block);
                PARFAIT_THROW("Could not find block owner");
            };
            BlockIJK owner_bijk = getOwnerBIJK();
            for (const auto& bijk : p.second) {
                if (std::get<0>(bijk) != owning_block) local_sync[bijk] = owner_bijk;
            }
        }
        return local_sync;
    }
};
}