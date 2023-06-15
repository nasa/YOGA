#pragma once
#include <parfait/Point.h>
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/MeshInterface.h>
#include <array>
#include <memory>
#include <set>
#include <vector>

namespace inf {

namespace Distance {
    struct MetaData {
        long gcid;
        int owner;
    };
}

std::vector<double> calculateDistance(MessagePasser mp,
                                      const inf::MeshInterface& mesh,
                                      std::set<int> tags,
                                      int max_depth = 10,
                                      int max_objects_per_voxel = 20);

std::vector<double> calcDistance(MessagePasser mp,
                                 const inf::MeshInterface& mesh,
                                 std::set<int> tags,
                                 const std::vector<Parfait::Point<double>>& points,
                                 int max_depth = 10,
                                 int max_objects_per_voxel = 20);

std::vector<Parfait::Point<double>> calcNearestSurfacePoint(
    MessagePasser mp,
    const MeshInterface& mesh,
    std::set<int> tags,
    const std::vector<Parfait::Point<double>>& points,
    int max_depth = 10,
    int max_objects_per_voxel = 20);

std::vector<std::tuple<Parfait::Point<double>, Distance::MetaData>> calculateDistanceAndMetaData(
    MessagePasser mp,
    const inf::MeshInterface& mesh,
    std::set<int> tags,
    int max_depth = 10,
    int max_objects_per_voxel = 20);
}
