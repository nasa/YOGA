#pragma once
#include "PluginLoader.h"
#include <mpi.h>
#include <memory>
#include "StructuredMesh.h"

namespace inf {
class StructuredMeshLoader {
  public:
    virtual std::shared_ptr<StructuredMesh> load(MPI_Comm comm, std::string filename) const = 0;
    virtual ~StructuredMeshLoader() = default;
};

inline std::shared_ptr<StructuredMeshLoader> getStructuredMeshLoader(std::string directory,
                                                                     std::string name) {
    return PluginLoader<StructuredMeshLoader>::loadPlugin(
        directory, name, "createStructuredMeshLoader");
}
}
