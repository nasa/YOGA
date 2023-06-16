#pragma once
#include <parfait/ExtentBuilder.h>
#include <parfait/ParallelExtent.h>
#include "MeshInterface.h"

namespace inf {
inline Parfait::Extent<double> partitionExtent(const inf::MeshInterface& mesh) {
    auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    for (int n = 0; n < mesh.nodeCount(); n++) {
        Parfait::Point<double> p = mesh.node(n);
        Parfait::ExtentBuilder::add(e, p);
    }
    return e;
}

inline Parfait::Extent<double> meshExtent(MessagePasser mp, const inf::MeshInterface& mesh) {
    auto e = partitionExtent(mesh);
    return Parfait::ParallelExtent::getBoundingBox(mp, e);
}
}
