#pragma once
#include <mpi.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "MeshInterface.h"
#include "PluginLoader.h"

namespace inf {
class Geometry {
  public:
    virtual std::array<double, 3> snap(double x, double y, double z) const = 0;
    virtual ~Geometry() = default;
};

class MeshDeformerInterface {
  public:
    virtual std::vector<double> deform(const std::map<int, std::array<double, 3>>& node_deflections,
                                       const std::vector<bool>& is_frozen,
                                       const std::map<int, Geometry*> geometries) const = 0;
    virtual ~MeshDeformerInterface() = default;
};

class MeshDeformerFactory {
  public:
    virtual std::shared_ptr<MeshDeformerInterface> createMeshDeformer(
        std::shared_ptr<MeshInterface> mesh, std::string json_config, MPI_Comm comm) const = 0;
    virtual ~MeshDeformerFactory() = default;
};

inline auto getMeshDeformerFactory(const std::string& directory, const std::string& name) {
    return PluginLoader<MeshDeformerFactory>::loadPlugin(directory, name, "createMeshDeformer");
}
}
