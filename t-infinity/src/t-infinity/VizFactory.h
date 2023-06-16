#pragma once
#include <mpi.h>
#include <memory>
#include <string>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/VizInterface.h>
#include "PluginLoader.h"
namespace inf {
class VizFactory {
  public:
    virtual std::shared_ptr<inf::VizInterface> create(std::string name,
                                                      std::shared_ptr<inf::MeshInterface> mesh,
                                                      MPI_Comm comm) const = 0;
    virtual ~VizFactory() = default;
};
inline auto getVizFactory(std::string directory, std::string name) {
    return PluginLoader<VizFactory>::loadPlugin(directory, name, "createVizFactory");
}
}
