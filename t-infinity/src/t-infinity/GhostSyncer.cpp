#include "GhostSyncer.h"

namespace inf {

GhostSyncer::GhostSyncer(MessagePasser mp, std::shared_ptr<MeshInterface> mesh)
    : mp(mp), mesh(mesh), node_sync_pattern(nullptr), cell_sync_pattern(nullptr) {}

void GhostSyncer::initializeNodeSyncing() {
    g2l_node = getNodeGlobalToLocal();

    std::vector<long> have, need;
    int me = mp.Rank();
    for (int n = 0; n < mesh->nodeCount(); n++)
        if (mesh->nodeOwner(n) == me)
            have.push_back(mesh->globalNodeId(n));
        else
            need.push_back(mesh->globalNodeId(n));
    node_sync_pattern =
        std::make_shared<Parfait::SyncPattern>(Parfait::SyncPattern(mp, have, need));
}

void GhostSyncer::initializeCellSyncing() {
    g2l_cell = getCellGlobalToLocal();

    std::vector<long> have, need;
    int me = mp.Rank();
    for (int n = 0; n < mesh->cellCount(); n++)
        if (mesh->cellOwner(n) == me)
            have.push_back(mesh->globalCellId(n));
        else
            need.push_back(mesh->globalCellId(n));
    cell_sync_pattern =
        std::make_shared<Parfait::SyncPattern>(Parfait::SyncPattern(mp, have, need));
}

std::map<long, int> GhostSyncer::getNodeGlobalToLocal() const {
    std::map<long, int> global_to_local;
    for (int n = 0; n < mesh->nodeCount(); n++) global_to_local[mesh->globalNodeId(n)] = n;
    return global_to_local;
}

std::map<long, int> GhostSyncer::getCellGlobalToLocal() const {
    std::map<long, int> global_to_local;
    for (int n = 0; n < mesh->cellCount(); n++) global_to_local[mesh->globalCellId(n)] = n;
    return global_to_local;
}
void GhostSyncer::printSyncPattern(Parfait::SyncPattern& pattern) {
    for (auto& pair : pattern.receive_from) {
        printf("%d recv from %d\n", mp.Rank(), pair.first);
    }

    for (auto& pair : pattern.send_to) {
        printf("%d send to %d\n", mp.Rank(), pair.first);
    }
    mp.Barrier();
}

}