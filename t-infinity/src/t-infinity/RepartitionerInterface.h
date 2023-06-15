#pragma once
#include <memory>
#include <t-infinity/MeshInterface.h>
#include "PluginLoader.h"

namespace inf {
class RepartitionerInterface {
  public:
    virtual std::shared_ptr<inf::MeshInterface> repartition(MPI_Comm comm,
                                                            const inf::MeshInterface& mesh) = 0;
    virtual ~RepartitionerInterface() = default;
};
inline auto getRepartitioner(std::string directory, std::string name) {
    return PluginLoader<RepartitionerInterface>::loadPlugin(directory, name, "createRepartitioner");
}
}
