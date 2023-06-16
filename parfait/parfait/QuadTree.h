#pragma once
#include "Point.h"
#include "Extent.h"

namespace Parfait {
namespace Quadtree {
    inline Parfait::Extent<double> getChild0Extent(const Parfait::Extent<double> e) {
        auto child_extent = e;
        child_extent.hi[0] = 0.5 * (e.lo[0] + e.hi[0]);
        child_extent.hi[1] = 0.5 * (e.lo[1] + e.hi[1]);
        return child_extent;
    }
    inline Parfait::Extent<double> getChild1Extent(const Parfait::Extent<double> e) {
        auto child_extent = e;
        child_extent.lo[0] = 0.5 * (e.lo[0] + e.hi[0]);
        child_extent.hi[1] = 0.5 * (e.lo[1] + e.hi[1]);
        return child_extent;
    }
    inline Parfait::Extent<double> getChild2Extent(const Parfait::Extent<double> e) {
        auto child_extent = e;
        child_extent.lo[0] = 0.5 * (e.lo[0] + e.hi[0]);
        child_extent.lo[1] = 0.5 * (e.lo[1] + e.hi[1]);
        return child_extent;
    }
    inline Parfait::Extent<double> getChild3Extent(const Parfait::Extent<double> e) {
        auto child_extent = e;
        child_extent.hi[0] = 0.5 * (e.lo[0] + e.hi[0]);
        child_extent.lo[1] = 0.5 * (e.lo[1] + e.hi[1]);
        return child_extent;
    }
}
}
