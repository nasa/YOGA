#pragma once
#include <t-infinity/MeshLoader.h>
#include <memory>

class NC_GrpPreProcessor : public inf::MeshLoader {
  public:
    std::shared_ptr<inf::MeshInterface> load(MPI_Comm comm, std::string filename) const override;
    void enableSanityChecks();
    void disableSanityChecks();
    void enableCacheReordering();
    void disableCacheReordering();

  private:
    bool run_sanity_checks = false;
    bool reorder_cache_efficiency = false;
};

extern "C" {
std::shared_ptr<inf::MeshLoader> createPreProcessor();
}
