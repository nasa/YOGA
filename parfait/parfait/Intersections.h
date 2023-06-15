#pragma once
#include "Facet.h"
#include "Point.h"

namespace Parfait {
namespace Intersections {
    inline double edgeAndPlane(const Parfait::Point<double>& a,
                               const Parfait::Point<double>& b,
                               const Parfait::Point<double>& point_on_plane,
                               const Parfait::Point<double>& plane_normal) {
        double d = Parfait::Point<double>::dot(plane_normal, point_on_plane);
        auto ray = b - a;
        if (Parfait::Point<double>::dot(ray, plane_normal) == 0)  // plane is aligned with the edge
            return -1.0;
        double x = (d - Parfait::Point<double>::dot(plane_normal, a)) / Parfait::Point<double>::dot(plane_normal, ray);
        return 1.0 - x;
    }

    inline std::array<double, 3> triangleAndRay(const Parfait::Point<double>& a,
                                                const Parfait::Point<double>& b,
                                                const Parfait::Point<double>& c,
                                                const Parfait::Point<double>& origin,
                                                const Parfait::Point<double>& ray) {
        Parfait::Facet f(a, b, c);
        Parfait::Point<double> p;
        double t;
        if (f.DoesRayIntersect(origin, ray, t)) {
            p = origin + t * ray;
            double abc = f.area();
            double cpb = Facet(c, p, b).area();
            double apc = Facet(a, p, c).area();
            double abp = Facet(a, b, p).area();

            return {cpb / abc, apc / abc, abp / abc};
        } else {
            return {-1, -1, -1};
        }
    }
}
}
