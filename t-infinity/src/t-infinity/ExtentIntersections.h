#pragma once
#include <parfait/Extent.h>

namespace inf {
inline bool sphereExtentIntersection(double radius,
                                     const Parfait::Point<double> center,
                                     const Parfait::Extent<double>& e) {
    auto closest_point = e.clamp(center);
    if (e.intersects(center))
        return true;
    else
        return (closest_point - center).magnitude() <= radius;
}
}
