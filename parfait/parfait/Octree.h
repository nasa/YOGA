#pragma once
#include "Extent.h"

namespace Parfait {
namespace Octree {
    inline Parfait::Extent<double> getChild0Extent(const Parfait::Extent<double>& extent) {
        Parfait::Extent<double> childExtent = extent;
        childExtent.hi[0] = 0.5 * (childExtent.lo[0] + childExtent.hi[0]);
        childExtent.hi[1] = 0.5 * (childExtent.lo[1] + childExtent.hi[1]);
        childExtent.hi[2] = 0.5 * (childExtent.lo[2] + childExtent.hi[2]);
        return childExtent;
    }
    inline Parfait::Extent<double> getChild1Extent(const Parfait::Extent<double>& extent) {
        Parfait::Extent<double> childExtent = extent;
        childExtent.lo[0] = 0.5 * (childExtent.lo[0] + childExtent.hi[0]);
        childExtent.hi[1] = 0.5 * (childExtent.lo[1] + childExtent.hi[1]);
        childExtent.hi[2] = 0.5 * (childExtent.lo[2] + childExtent.hi[2]);
        return childExtent;
    }
    inline Parfait::Extent<double> getChild2Extent(const Parfait::Extent<double>& extent) {
        Parfait::Extent<double> childExtent = extent;
        childExtent.lo[0] = 0.5 * (childExtent.lo[0] + childExtent.hi[0]);
        childExtent.lo[1] = 0.5 * (childExtent.lo[1] + childExtent.hi[1]);
        childExtent.hi[2] = 0.5 * (childExtent.lo[2] + childExtent.hi[2]);
        return childExtent;
    }
    inline Parfait::Extent<double> getChild3Extent(const Parfait::Extent<double>& extent) {
        Parfait::Extent<double> childExtent = extent;
        childExtent.lo[1] = 0.5 * (childExtent.lo[1] + childExtent.hi[1]);
        childExtent.hi[0] = 0.5 * (childExtent.lo[0] + childExtent.hi[0]);
        childExtent.hi[2] = 0.5 * (childExtent.lo[2] + childExtent.hi[2]);
        return childExtent;
    }
    inline Parfait::Extent<double> getChild4Extent(const Parfait::Extent<double>& extent) {
        Parfait::Extent<double> childExtent = extent;
        childExtent.hi[0] = 0.5 * (childExtent.lo[0] + childExtent.hi[0]);
        childExtent.hi[1] = 0.5 * (childExtent.lo[1] + childExtent.hi[1]);
        childExtent.lo[2] = 0.5 * (childExtent.lo[2] + childExtent.hi[2]);
        return childExtent;
    }
    inline Parfait::Extent<double> getChild5Extent(const Parfait::Extent<double>& extent) {
        Parfait::Extent<double> childExtent = extent;
        childExtent.lo[0] = 0.5 * (childExtent.lo[0] + childExtent.hi[0]);
        childExtent.hi[1] = 0.5 * (childExtent.lo[1] + childExtent.hi[1]);
        childExtent.lo[2] = 0.5 * (childExtent.lo[2] + childExtent.hi[2]);
        return childExtent;
    }
    inline Parfait::Extent<double> getChild6Extent(const Parfait::Extent<double>& extent) {
        Parfait::Extent<double> childExtent = extent;
        childExtent.lo[0] = 0.5 * (childExtent.lo[0] + childExtent.hi[0]);
        childExtent.lo[1] = 0.5 * (childExtent.lo[1] + childExtent.hi[1]);
        childExtent.lo[2] = 0.5 * (childExtent.lo[2] + childExtent.hi[2]);
        return childExtent;
    }
    inline Parfait::Extent<double> getChild7Extent(const Parfait::Extent<double>& extent) {
        Parfait::Extent<double> childExtent = extent;
        childExtent.lo[1] = 0.5 * (childExtent.lo[1] + childExtent.hi[1]);
        childExtent.hi[0] = 0.5 * (childExtent.lo[0] + childExtent.hi[0]);
        childExtent.lo[2] = 0.5 * (childExtent.lo[2] + childExtent.hi[2]);
        return childExtent;
    }
    inline Parfait::Extent<double> childExtent(const Parfait::Extent<double>& extent, int child) {
        switch (child) {
            case 0:
                return getChild0Extent(extent);
            case 1:
                return getChild1Extent(extent);
            case 2:
                return getChild2Extent(extent);
            case 3:
                return getChild3Extent(extent);
            case 4:
                return getChild4Extent(extent);
            case 5:
                return getChild5Extent(extent);
            case 6:
                return getChild6Extent(extent);
            case 7:
                return getChild7Extent(extent);
            default:
                throw std::logic_error("Voxels only have 8 children");
        }
    }
}

}
