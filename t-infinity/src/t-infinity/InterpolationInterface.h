#pragma once
#include <mpi.h>
#include "FieldInterface.h"
#include "MeshInterface.h"
#include "PluginLoader.h"

namespace inf {
class InterpolationInterface {
  public:
    virtual std::shared_ptr<FieldInterface> interpolate(
        std::shared_ptr<FieldInterface> donor_values) const = 0;
    inline virtual void addDebugFields(std::vector<std::shared_ptr<FieldInterface>>& fields) {}
    virtual ~InterpolationInterface() = default;
};

class InterpolationFactory {
  public:
    virtual std::shared_ptr<InterpolationInterface> create(std::shared_ptr<MeshInterface> donor,
                                                           std::shared_ptr<MeshInterface> receptor,
                                                           MPI_Comm comm) const = 0;
    virtual ~InterpolationFactory() = default;
};

inline auto getInterpolator(const std::string& directory,
                            const std::string& name,
                            MPI_Comm comm,
                            std::shared_ptr<MeshInterface> donor,
                            std::shared_ptr<MeshInterface> receptor) {
    return PluginLoader<InterpolationFactory>::loadPluginAndCreate(
        directory, name, "createInterpolationFactory", donor, receptor, comm);
}
}
