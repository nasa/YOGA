#pragma once

#include "Point.h"
#include "Extent.h"
#include "Metrics.h"
#include "ExtentBuilder.h"
#include "Facetize.h"
#include "STL.h"

namespace Parfait {
class Region {
  public:
    virtual bool contains(const Parfait::Point<double>& p) const = 0;
    virtual ~Region() = default;
};

class ExtentRegion : public Region {
  public:
    inline ExtentRegion(Parfait::Extent<double> e) : extent(e) {}
    inline bool contains(const Parfait::Point<double>& p) const override { return extent.intersects(p); }

  public:
    Parfait::Extent<double> extent;
};

class SphereRegion : public Region {
  public:
    inline SphereRegion(const Parfait::Point<double>& center, double radius) : center(center), radius(radius) {}

    inline bool contains(const Parfait::Point<double>& p) const override {
        auto diff = p - center;
        auto length = diff.magnitude();
        return length <= radius;
    }

  public:
    Parfait::Point<double> center;
    double radius;
};

class TetRegion : public Region {
  public:
    inline TetRegion(std::array<Parfait::Point<double>, 4> corners) : corners(corners) {
        extent = Parfait::ExtentBuilder::build(corners);
    }

    inline bool contains(const Parfait::Point<double>& p) const override {
        if (not extent.intersects(p)) return false;
        using namespace Parfait::Metrics;
        auto vol = computeTetVolume(corners[0], corners[1], corners[2], p);
        if (vol < 0.0) return false;
        vol = computeTetVolume(corners[0], corners[2], corners[3], p);
        if (vol < 0.0) return false;
        vol = computeTetVolume(corners[0], corners[3], corners[1], p);
        if (vol < 0.0) return false;
        vol = computeTetVolume(corners[1], corners[3], corners[2], p);
        if (vol < 0.0) return false;

        return true;
    }

  public:
    std::array<Parfait::Point<double>, 4> corners;
    Parfait::Extent<double> extent;
};

class HexRegion : public Region {
  public:
    inline HexRegion(std::array<Parfait::Point<double>, 8>& corners) {
        extent = Parfait::ExtentBuilder::build(corners);
        tets[0] = {corners[0], corners[1], corners[2], corners[5]};
        tets[1] = {corners[0], corners[2], corners[7], corners[5]};
        tets[2] = {corners[0], corners[2], corners[3], corners[7]};
        tets[3] = {corners[0], corners[5], corners[7], corners[4]};
        tets[4] = {corners[2], corners[7], corners[5], corners[6]};
    }

    inline bool contains(const Parfait::Point<double>& p) const {
        if (not extent.intersects(p)) return false;
        for (auto& tet : tets) {
            TetRegion t(tet);
            if (t.contains(p)) return true;
        }
        return false;
    }

  public:
    Parfait::Extent<double> extent;
    std::array<std::array<Parfait::Point<double>, 4>, 5> tets;
};

class CompositeRegion : public Region {
  public:
    inline bool contains(const Parfait::Point<double>& p) const {
        for (auto& r : regions) {
            if (r->contains(p)) return true;
        }
        return false;
    }
    inline void add(std::shared_ptr<Region> r) { regions.emplace_back(r); }
    std::vector<std::shared_ptr<Region>> regions;
};

}