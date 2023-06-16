#pragma once
#include "MeshInterface.h"
#include "PluginLoader.h"
#include <parfait/Point.h>
#include <MessagePasser/MessagePasser.h>

namespace inf {
class DistanceCalculatorInterface {
  public:
    virtual std::vector<double> calculateDistanceToNodes(MessagePasser mp,
                                                         const std::set<int>& tags,
                                                         const MeshInterface& mesh) const = 0;
    virtual ~DistanceCalculatorInterface() = default;
};
inline auto getDistanceCalculator(const std::string& directory, const std::string& name) {
    return PluginLoader<DistanceCalculatorInterface>::loadPlugin(
        directory, name, "createDistanceCalculator");
}
}
