
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
#pragma once

#include <parfait/Point.h>

namespace Parfait {
template <typename T>
class Plane {
  public:
    enum Orientation { XY, XZ, YZ };
    Plane() = default;

    PARFAIT_DEVICE Plane(const Point<T>& plane_normal) : unit_normal(plane_normal / plane_normal.magnitude()) {}

    PARFAIT_DEVICE Plane(const Point<T>& plane_normal, const Point<T>& origin)
        : unit_normal(plane_normal / plane_normal.magnitude()), origin(origin) {}
    explicit Plane(Orientation orientation) : unit_normal(generate(orientation)) {}
    PARFAIT_DEVICE Point<T> reflect(Point<T>& p) {
        return p - unit_normal * T(2.0) * Parfait::Point<T>::dot(p, unit_normal);
    }

    PARFAIT_DEVICE double edgeIntersectionWeight(const Point<T>& p1, const Point<T>& p2) {
        // From https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection
        // returns the interpolation weight w such that the intersection point can be found by:
        // intersection = w * p1 + (1.0 - w) * p2
        auto l = p2 - p1;
        auto a = Point<T>::dot((origin - p1), unit_normal);
        auto b = Point<T>::dot(l, unit_normal);
        auto d = a / b;
        return 1.0 - d;
    }

  public:
    Point<T> unit_normal;
    Point<T> origin;
    Point<T> generate(Orientation orientation) {
        if (XY == orientation) return {0, 0, 1};
        if (XZ == orientation) return {0, 1, 0};
        if (YZ == orientation) return {1, 0, 0};
        throw std::logic_error("Invalid plane orientation");
    }

    inline bool isOrientation(Orientation o, double tol = 1.0e-10) {
        auto n = generate(o);
        auto dot = Parfait::Point<T>::dot(n, unit_normal);
        return dot > (1.0 - tol);
    }
    PARFAIT_DEVICE inline Point<T> orthogonalize(const Point<T>& v1, Point<T> v2) {
        v2 = v2 - Parfait::Point<T>::dot(v1, v2) * v1;
        return v2;
    }
    std::tuple<Point<T>, Point<T>> orthogonalVectors(double tolerance = 1.0e-10) {
        if (isOrientation(YZ, tolerance)) {
            return {{0, 1, 0}, {0, 0, 1}};
        } else if (isOrientation(XZ, tolerance)) {
            return {{1, 0, 0}, {0, 0, 1}};
        } else if (isOrientation(XY, tolerance)) {
            return {{1, 0, 0}, {0, 1, 0}};
        }
        Point<T> v1 = {1, 1, 1};
        v1.normalize();
        if (Parfait::Point<T>::dot(v1, unit_normal) > (1.0 - tolerance)) {
            v1 = {1, 1, 0};
            v1.normalize();
        }
        v1 = orthogonalize(unit_normal, v1);
        v1.normalize();
        auto v2 = Parfait::Point<T>::cross(unit_normal, v1);
        return {v1, v2};
    }
};
}
