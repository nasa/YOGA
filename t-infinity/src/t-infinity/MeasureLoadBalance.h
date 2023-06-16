#pragma once

#include <t-infinity/MeshInterface.h>
#include <MessagePasser/MessagePasser.h>

namespace inf {
namespace LoadBalance {
    void printUnweightedCellStatistics(MessagePasser mp, const MeshInterface& mesh);
    void printLoadBalaceStatistics(MessagePasser mp, const MeshInterface& mesh);
    void printStatistics(MessagePasser mp, std::string name, double rank_measurement);
}
}