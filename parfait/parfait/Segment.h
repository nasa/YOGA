#pragma once
#include "Point.h"
#include "Extent.h"
#include "ExtentBuilder.h"
#include "Throw.h"

namespace Parfait {
template <typename T, size_t N = 3>
class Segment {
  public:
    Segment(const Parfait::Point<T, N>& a, const Parfait::Point<T, N>& b) : a(a), b(b) {}

    auto length() const -> T { return (b - a).magnitude(); }
    auto getExtent() const -> Extent<T> {
        auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<T>();
        Parfait::ExtentBuilder::add(e, a);
        Parfait::ExtentBuilder::add(e, b);
        return e;
    }
    auto getClosestPoint(const Point<T, N> p) const -> Point<T, N> {
        auto ab = b - a;
        if (ab.magnitude() < 1.0e-16) throw std::logic_error("Line is degenerate");
        double t = Point<T, N>::dot(p - a, ab) / Point<T, N>::dot(ab, ab);
        if (t < 0) return a;
        if (t > 1.0) return b;
        return a + ab * t;
    }
    void getNode(int i, T* p) const {
        switch (i) {
            case 0:
                for (int i = 0; i < N; i++) p[i] = a[i];
                return;
            case 1:
                for (int i = 0; i < N; i++) p[i] = b[i];
                return;
            default:
                PARFAIT_THROW("Segments only have 2 nodes");
        }
    }
    auto getNode(int i) const -> Point<double> {
        Point<double> p;
        getNode(i, p.data());
        return p;
    }

  public:
    Parfait::Point<T, N> a;
    Parfait::Point<T, N> b;
};
}