#include "GhostSyncPatternBuilder.h"

namespace YOGA {
Parfait::SyncPattern GhostSyncPatternBuilder::build(const YogaMesh& mesh,
                                                    MessagePasser mp) {
    std::vector<long> have, need;
    std::vector<int> need_from;
    for (int i = 0; i < mesh.nodeCount(); i++) {
        if (mp.Rank() == mesh.nodeOwner(i)) {
            have.push_back(mesh.globalNodeId(i));
        } else {
            need.push_back(mesh.globalNodeId(i));
            need_from.push_back(mesh.nodeOwner(i));
        }
    }
    return Parfait::SyncPattern(mp, have, need, need_from);
}
}