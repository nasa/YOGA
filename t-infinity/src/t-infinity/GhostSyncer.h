#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/SyncField.h>
#include <parfait/SyncPattern.h>
#include <t-infinity/MeshInterface.h>
#include <map>
#include <memory>
#include <vector>

namespace inf {
class GhostSyncer {
  public:
    GhostSyncer(MessagePasser mp, std::shared_ptr<MeshInterface> mesh);

    void initializeNodeSyncing();
    void initializeCellSyncing();

    template <typename T>
    void syncNodes(std::vector<T>& vec) {
        if (node_sync_pattern == nullptr) initializeNodeSyncing();
        Parfait::SyncWrapper<T> syncer(vec, g2l_node);
        Parfait::syncField<T>(mp, syncer, *node_sync_pattern);
    }

    void printSyncPattern(Parfait::SyncPattern& pattern);
    template <typename T>
    void syncCells(std::vector<T>& vec) {
        if (cell_sync_pattern == nullptr) initializeCellSyncing();
        Parfait::SyncWrapper<T> syncer(vec, g2l_cell);
        Parfait::syncField<T>(mp, syncer, *cell_sync_pattern);
    }

  private:
    MessagePasser mp;
    std::shared_ptr<MeshInterface> mesh;
    std::shared_ptr<Parfait::SyncPattern> node_sync_pattern;
    std::shared_ptr<Parfait::SyncPattern> cell_sync_pattern;

    std::map<long, int> g2l_node;
    std::map<long, int> g2l_cell;

    std::map<long, int> getNodeGlobalToLocal() const;
    std::map<long, int> getCellGlobalToLocal() const;
};

}
