#include "DistanceCalculatorPlugin.h"
#include <t-infinity/BetterDistanceCalculator.h>

std::vector<double> ParfaitPlugins::DistanceCalculatorPlugin::calculateDistanceToNodes(
    MessagePasser mp, const std::set<int>& tags, const inf::MeshInterface& mesh) const {
    std::vector<double> distance_to_nodes;
    std::tie(distance_to_nodes, std::ignore, std::ignore) = inf::calcDistanceToNodes(mp, mesh, tags);
    return distance_to_nodes;
}
std::shared_ptr<inf::DistanceCalculatorInterface> createDistanceCalculator() {
    return std::make_shared<ParfaitPlugins::DistanceCalculatorPlugin>();
}
