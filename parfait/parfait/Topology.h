#pragma once
#include <parfait/Throw.h>
#include <map>
#include <vector>
#include <set>

namespace Parfait {
class Topology {
  public:
    inline Topology(const std::vector<long>& gids, const std::vector<bool>& do_own) : global_ids(gids), do_own(do_own) {
        if (global_ids.size() != do_own.size()) {
            PARFAIT_THROW("Topology must be made with ownership known for each GID. ");
        }
        buildG2L();
    }

  public:
    std::vector<long> global_ids;
    std::vector<bool> do_own;
    // You are allowed to have multiple local ids
    // map to the same global id.
    // So long as they are ghosts - meaning you don't own them.
    std::map<long, std::set<int>> global_to_local;

    inline void buildG2L() {
        for (int local = 0; local < int(global_ids.size()); local++) {
            auto global = global_ids[local];
            if (do_own[local]) {
                if (global_to_local.count(global) != 0) {
                    PARFAIT_THROW("Two owned local ids map to the same GID");
                }
            }
            global_to_local[global].insert(local);
        }
    }
};
}
