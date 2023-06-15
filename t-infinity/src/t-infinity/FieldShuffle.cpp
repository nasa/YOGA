#include "FieldShuffle.h"
#include "Extract.h"

namespace inf {
namespace FieldShuffle {
    Parfait::SyncPattern buildMeshToMeshNodeSyncPattern(const MessagePasser& mp,
                                                        const MeshInterface& source,
                                                        const MeshInterface& target) {
        auto have = extractOwnedGlobalNodeIds(source);
        auto need = extractGlobalNodeIds(target);
        Parfait::SyncPattern syncer(mp, have, need);
        return syncer;
    }
    std::vector<long> buildLocalToPreviousGlobalNode(const MeshInterface& source,
                                                     const CellSelectedMesh& target) {
        std::vector<long> local_to_previous_global(target.nodeCount());
        for (int node_id = 0; node_id < target.nodeCount(); ++node_id) {
            int source_local_id = target.previousNodeId(node_id);
            local_to_previous_global[node_id] = source.globalNodeId(source_local_id);
        }
        return local_to_previous_global;
    }
}
}