#pragma once
#include <mpi.h>
#include <memory>
#include "MeshInterface.h"
#include "PluginLoader.h"
namespace inf {
class MeshExtruderInterface {
  public:
    virtual std::shared_ptr<MeshInterface> extrude(std::shared_ptr<MeshInterface> mesh,
                                                   MPI_Comm comm) const = 0;
    virtual std::shared_ptr<MeshInterface> revolve(std::shared_ptr<MeshInterface> mesh,
                                                   MPI_Comm comm) const = 0;
    virtual ~MeshExtruderInterface() = default;
};

inline auto getMeshExtruder(std::string directory, std::string name) {
    return PluginLoader<MeshExtruderInterface>::loadPlugin(directory, name, "createMeshExtruder");
}
}
