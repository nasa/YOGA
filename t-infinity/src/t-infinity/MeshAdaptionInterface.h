#pragma once
#include <mpi.h>
#include <memory>
#include <string>
#include "GeometryAssociationInterface.h"
#include "ByteStreamInterface.h"
#include "FieldInterface.h"
#include "MeshInterface.h"
#include "PluginLoader.h"

namespace inf {
class MeshAdaptationInterface {
  public:
    virtual std::shared_ptr<MeshInterface> adapt(std::shared_ptr<MeshInterface> mesh,
                                                 MPI_Comm comm,
                                                 std::shared_ptr<FieldInterface> metric,
                                                 std::string json_options) const = 0;

    struct MeshAssociatedWithGeometry {
        std::shared_ptr<MeshInterface> mesh;
        std::shared_ptr<GeometryAssociationInterface> mesh_geometry_association;
    };

    virtual MeshAssociatedWithGeometry adapt(
        MeshAssociatedWithGeometry mesh_associated_with_geometry,
        std::shared_ptr<ByteStreamInterface> geometry_model,
        MPI_Comm comm,
        std::shared_ptr<FieldInterface> metric,
        std::string json_options) const = 0;

    virtual ~MeshAdaptationInterface() = default;
};

inline auto getMeshAdaptationPlugin(const std::string& directory, const std::string& name) {
    return PluginLoader<MeshAdaptationInterface>::loadPlugin(
        directory, name, "createMeshAdaptation");
}
}
