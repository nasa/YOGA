#pragma once
#include <t-infinity/MeshLoader.h>
#include <MessagePasser/MessagePasser.h>
#include <memory>

class SerialPreProcessor : public inf::MeshLoader {
  public:
    std::shared_ptr<inf::MeshInterface> load(MPI_Comm comm, std::string filename) const override;
};

extern "C" {
std::shared_ptr<inf::MeshLoader> createPreProcessor();
}
