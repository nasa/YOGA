#pragma once

#include <string>
#include "StructuredMesh.h"
#include "PluginLoader.h"

namespace inf {
class StructuredMeshAdaptationInterface {
  public:
    virtual void adapt(StructuredBlock& block,
                       const StructuredBlockField& metric,
                       std::string json_options) const = 0;
    virtual ~StructuredMeshAdaptationInterface() = default;
};
inline auto getStructuredMeshAdaptationPlugin(const std::string& directory,
                                              const std::string& name) {
    return PluginLoader<StructuredMeshAdaptationInterface>::loadPlugin(
        directory, name, "createStructuredMeshAdaptation");
}
}