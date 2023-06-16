#pragma once
#include "Extent.h"
#include "Facet.h"
#include "Point.h"
#include "Plane.h"

namespace Parfait {

namespace Polygons {
    typedef std::vector<Parfait::Point<double>> Polygon;
    typedef std::vector<Polygon> Polyhedron;
    inline bool planeClipping(const Plane<double>& p,
                              Parfait::Point<double> a,
                              Parfait::Point<double> b,
                              Parfait::Point<double>& intersection_location) {
        auto l = b - a;
        double edge_length = l.magnitude();
        l.normalize();
        auto l_dot_n = Parfait::Point<double>::dot(p.unit_normal, l);
        if (l_dot_n == 0) {
            return false;
        }
        auto d = Parfait::Point<double>::dot((p.origin - a), p.unit_normal) / l_dot_n;
        if (d > edge_length or d < 0) {
            return false;
        }
        intersection_location = a + d * l;
        return true;
    }

    inline bool inFront(const Parfait::Plane<double>& plane, Parfait::Point<double> a) {
        auto l = a - plane.origin;
        l.normalize();
        return Parfait::Point<double>::dot(l, plane.unit_normal) > 0;
    }

    inline Polygon planeClipping(const Plane<double>& plane, Polygon polygon) {
        Polygon out;
        for (size_t i = 0; i < polygon.size(); i++) {
            auto a = polygon[i];
            auto b = polygon[(i + 1) % polygon.size()];
            Parfait::Point<double> p;
            if (inFront(plane, a)) out.push_back(a);
            if (planeClipping(plane, a, b, p)) {
                out.push_back(p);
            }
        }
        return out;
    }
    inline Polyhedron planeClipping(const Plane<double>& plane, const Polyhedron& polygons) {
        std::vector<Polygon> out;
        for (auto& p : polygons) {
            auto clipped = planeClipping(plane, p);
            if (not clipped.empty()) {
                out.push_back(clipped);
            }
        }
        return out;
    }

    inline Polygon intersection(const Parfait::Extent<double>& e, Parfait::Facet& f) {
        Polygon polygon;
        polygon.push_back(f[0]);
        polygon.push_back(f[1]);
        polygon.push_back(f[2]);

        polygon = planeClipping(Plane<double>{{1, 0, 0}, e.lo}, polygon);
        polygon = planeClipping(Plane<double>{{0, 1, 0}, e.lo}, polygon);
        polygon = planeClipping(Plane<double>{{0, 0, 1}, e.lo}, polygon);

        polygon = planeClipping(Plane<double>{{-1, 0, 0}, e.hi}, polygon);
        polygon = planeClipping(Plane<double>{{0, -1, 0}, e.hi}, polygon);
        polygon = planeClipping(Plane<double>{{0, 0, -1}, e.hi}, polygon);
        return polygon;
    }

    inline std::vector<Parfait::Facet> triangulate(const Polygon& convex_polygon) {
        std::vector<Parfait::Facet> tris;
        if (convex_polygon.size() < 3) PARFAIT_THROW("Can't tesselate polygon that isn't at least a triangle");
        for (size_t i = 1; i < convex_polygon.size() - 1; i++) {
            tris.emplace_back(Parfait::Facet{convex_polygon[0], convex_polygon[i], convex_polygon[i + 1]});
        }
        return tris;
    }

    inline std::vector<Parfait::Facet> triangulate(const std::vector<Polygon>& convex_polygons) {
        std::vector<Parfait::Facet> tris;
        for (auto& p : convex_polygons) {
            auto p_tris = triangulate(p);
            tris.insert(tris.end(), p_tris.begin(), p_tris.end());
        }
        return tris;
    }

    inline Polygon extentSideAsPolygon(const Parfait::Extent<double>& e, int side) {
        auto c = e.corners();
        Polygon p;
        p.reserve(4);
        switch (side) {
            case 0: {
                p.emplace_back(c[0]);
                p.emplace_back(c[3]);
                p.emplace_back(c[2]);
                p.emplace_back(c[1]);
                return p;
            }
            case 1: {
                p.emplace_back(c[0]);
                p.emplace_back(c[1]);
                p.emplace_back(c[5]);
                p.emplace_back(c[4]);
                return p;
            }
            case 2: {
                p.emplace_back(c[1]);
                p.emplace_back(c[2]);
                p.emplace_back(c[6]);
                p.emplace_back(c[5]);
                return p;
            }
            case 3: {
                p.emplace_back(c[2]);
                p.emplace_back(c[3]);
                p.emplace_back(c[7]);
                p.emplace_back(c[6]);
                return p;
            }
            case 4: {
                p.emplace_back(c[0]);
                p.emplace_back(c[4]);
                p.emplace_back(c[7]);
                p.emplace_back(c[3]);
                return p;
            }
            case 5: {
                p.emplace_back(c[4]);
                p.emplace_back(c[5]);
                p.emplace_back(c[6]);
                p.emplace_back(c[7]);
                return p;
            }
            default: {
                PARFAIT_THROW("Extents only have 6 sides");
            }
        }
    }

    inline Polyhedron extentAsPolyhedron(const Parfait::Extent<double>& e) {
        Polyhedron polyhedron;
        for (int s = 0; s < 6; s++) {
            polyhedron.push_back(extentSideAsPolygon(e, s));
        }
        return polyhedron;
    }
}
}
