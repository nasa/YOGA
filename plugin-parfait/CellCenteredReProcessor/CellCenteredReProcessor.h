#pragma once

#include <MessagePasser/MessagePasser.h>
#include <t-infinity/MeshLoader.h>
#include <t-infinity/TinfMesh.h>

class CC_ReProcessor : public inf::MeshLoader {
  public:
    std::shared_ptr<inf::MeshInterface> load(MPI_Comm comm, std::string filename) const override;
    ~CC_ReProcessor() = default;

  private:
    mutable bool run_sanity_checks = false;
    mutable bool reorder_for_efficiency = false;
    mutable bool reorder_randomly = false;
    mutable bool use_hiro_centroid = false;
    mutable bool use_partition_diffusion = false;
    mutable bool use_q_reordering = false;
    mutable std::string partitioner = "default";
    mutable double q_reordering_prune_width = 1.0;
};

extern "C" {
std::shared_ptr<inf::MeshLoader> createPreProcessor();
}
