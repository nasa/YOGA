#pragma once
#include "FieldShuffle.h"
#include "Subdomain.h"

namespace inf {
class MeshSubdomain {
  public:
    using Map = std::map<long, int>;
    MeshSubdomain(MPI_Comm comm,
                  std::shared_ptr<MeshInterface> mesh,
                  const std::vector<int>& subdomain_nodes);

    Parfait::SyncPattern buildOriginalToSubdomainSyncPattern() const;
    Parfait::SyncPattern buildSubdomainToOriginalSyncPattern() const;

    void balanceSubdomainNodes(const std::vector<double>& node_costs);
    void inflateSubdomainGhostNodes();

    template <typename T>
    void syncOriginalToSubdomain(T* source, T* target, int stride) const {
        auto sync_pattern = buildOriginalToSubdomainSyncPattern();
        auto syncer = FieldShuffle::buildMeshToMeshFieldStridedSyncer(
            mp, source, target, sync_pattern, original_g2l, subdomain_g2l, stride);
        syncer.start();
        syncer.finish();
    }

    template <typename T>
    void syncSubdomainToOriginal(T* source, T* target, int stride) const {
        auto sync_pattern = buildSubdomainToOriginalSyncPattern();
        auto syncer = FieldShuffle::buildMeshToMeshFieldStridedSyncer(
            mp, source, target, sync_pattern, subdomain_g2l, original_g2l, stride);
        syncer.start();
        syncer.finish();
    }

    MessagePasser mp;
    std::vector<int> subdomain_nodes_on_original_mesh;
    std::shared_ptr<MeshInterface> original_mesh;
    std::shared_ptr<MeshInterface> subdomain_mesh;
    Map original_g2l;
    Map subdomain_g2l;
};
}
