
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

#include "Point.h"

namespace Parfait {
template <typename T, size_t N = 3>
class Extent {
  public:
    Point<T, N> lo, hi;

    Extent();
    Extent(const T extent[N]);
    Extent(const Point<T, N>& lo, const Point<T, N>& hi);
    T* data() { return lo.data(); };
    const T* data() const { return lo.data(); };

    bool strictlyContains(const Extent& e) const;
    bool intersects(const Point<T, N>& p) const;
    bool intersects(const Extent& box) const;
    bool intersectsSegment(const Point<double, N>& a, const Point<double, N>& b) const;
    bool approxEqual(const Extent& e) const;
    T getLength(int d) const;
    T getLength_X() const;
    T getLength_Y() const;
    T getLength_Z() const;
    Point<T, N> center() const;
    std::array<Point<T>, 8> corners() const;
    Parfait::Extent<T, N> intersection(const Extent<T, N>& b) const;

    void resize(double scale);
    void resize(double scaleX, double scaleY, double scaleZ);
    double radius() const;
    Point<T, N> clamp(Point<T, N> p) const;

    void makeIsotropic();
    double distance(const Parfait::Point<T, N>& p) const;
    int longestDimension() const;

    std::string to_string() const;

    static Extent extentAroundPoint(const Point<T, N>& p, T tol);
    static double volume(const Extent& domain);
    double volume() const;
};

}

#include "Extent.hpp"