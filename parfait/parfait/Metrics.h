
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
#include <array>
#include <vector>
#include "Point.h"

namespace Parfait {
namespace Metrics {

    /*  Computes the volume of a tet given the xyz coords of its
      vertices.  It returns the signed volume, so in order for it
      to be positive, the nodes have to be in the expected order
      (matching ugrid format).

              D
             /|\
            / | \
           /  |  \
          /   |   \
         /    |    \
      A /     |     \ C
        \` ` `|` ` `/
         \    |    /
          \   |   /
           \  |  /
            \ | /
             \|/
              B

    */
    PARFAIT_INLINE double computeTetVolume(const Point<double>& a,
                                           const Point<double>& b,
                                           const Point<double>& c,
                                           const Point<double>& d) {
        auto v1 = a - d;
        auto v2 = b - d;
        auto v3 = c - d;
        auto v = Point<double>::cross(v2, v3);
        return -Point<double>::dot(v1, v) / 6.0;
    }

    PARFAIT_INLINE Parfait::Point<double> computeTriangleArea(const Parfait::Point<double>& a,
                                                              const Parfait::Point<double>& b,
                                                              const Parfait::Point<double>& c) {
        auto v1 = b - a;
        auto v2 = c - a;
        return Point<double>::cross(v1, v2) * 0.5;
    }

    inline Parfait::Point<double> average(const std::vector<Parfait::Point<double>>& points) {
        Parfait::Point<double> avg = {0, 0, 0};
        for (auto& p : points) avg += p;
        return avg / (double)points.size();
    }

    PARFAIT_INLINE void calcCornerAreas(Point<double>* corner_areas,
                                        const Point<double>* const corners,
                                        size_t NSides) {
        Point<double> centroid = {0, 0, 0};
        for (size_t i = 0; i < NSides; i++) {
            centroid += corners[i];
        }
        centroid *= 1.0 / double(NSides);

        for (size_t i = 0; i < NSides; i++) {
            long l_node = (i + NSides - 1) % NSides;
            long r_node = (i + 1) % NSides;
            const auto& n_point = corners[i];
            const auto& l_point = corners[l_node];
            const auto& r_point = corners[r_node];

            Point<double> l_edge_mid = (l_point + n_point) / 2.0;
            Point<double> r_edge_mid = (r_point + n_point) / 2.0;

            Point<double> area = {0, 0, 0};
            Point<double> u = r_edge_mid - n_point;
            Point<double> v = centroid - n_point;
            area += Point<double>::cross(u, v) / 2.0;

            u = v;
            v = l_edge_mid - n_point;
            area += Point<double>::cross(u, v) / 2.0;
            corner_areas[i] = area;
        }
    }

    // This function indirection is to avoid calling host-only the std::array [] operator inside calcCornerAreas
    template <class Array>
    Array calcCornerAreas(const Array& corners) {
        Array areas{};
        calcCornerAreas(areas.data(), corners.data(), corners.size());
        return areas;
    }

    PARFAIT_INLINE Point<double> XYLineNormal(const Point<double>& start, const Point<double>& end) {
        auto line = end - start;
        return Point<double>::cross(line, Point<double>{0, 0, 1});
    }
}
}
