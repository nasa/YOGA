#pragma once
#include <mpi.h>
#include <memory>
#include <string>
#include <t-infinity/MeshInterface.h>
#include "PluginLoader.h"

namespace inf {
class MeshLoader {
  public:
    virtual std::shared_ptr<MeshInterface> load(MPI_Comm comm, std::string mesh_filename) const = 0;
    virtual ~MeshLoader() = default;
};
inline auto getMeshLoader(const std::string& directory, const std::string& name) {
    return PluginLoader<MeshLoader>::loadPlugin(directory, name, "createPreProcessor");
}
}