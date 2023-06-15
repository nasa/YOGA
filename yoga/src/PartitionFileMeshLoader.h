#pragma once
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/MeshLoader.h>

using namespace inf;

class PartFileMeshLoader : public MeshLoader {
  public:
    PartFileMeshLoader() = default;
    std::shared_ptr<MeshInterface> load(MPI_Comm comm, std::string filename) const;

  private:
    const std::string partition_filename = "partition.data";

};

extern "C" {
std::shared_ptr<inf::MeshLoader> createPreProcessor();
}
