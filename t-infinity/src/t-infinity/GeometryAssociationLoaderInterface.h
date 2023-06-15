#pragma once
#include <mpi.h>
#include <memory>
#include <string>
#include "MeshInterface.h"
#include "GeometryAssociationInterface.h"

namespace inf {
class GeometryAssociationLoaderInterface {
  public:
    virtual std::shared_ptr<GeometryAssociationInterface> load(
        MPI_Comm comm,
        const std::string& filename,
        std::shared_ptr<inf::MeshInterface> mesh) const = 0;
    virtual ~GeometryAssociationLoaderInterface() = default;
};
inline auto getGeometryAssociationLoader(const std::string& directory, const std::string& name)
    -> std::shared_ptr<GeometryAssociationLoaderInterface> {
    return PluginLoader<GeometryAssociationLoaderInterface>::loadPlugin(
        directory, name, "createGeometryAssociationLoader");
}
}
