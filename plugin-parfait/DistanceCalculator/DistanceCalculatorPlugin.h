#pragma once
#include <t-infinity/DistanceCalculatorInterface.h>

namespace ParfaitPlugins {
class DistanceCalculatorPlugin : public inf::DistanceCalculatorInterface {
  public:
    std::vector<double> calculateDistanceToNodes(MessagePasser mp,
                                                 const std::set<int>& tags,
                                                 const inf::MeshInterface& mesh) const override;
};
}

extern "C" {
std::shared_ptr<inf::DistanceCalculatorInterface> createDistanceCalculator();
}
