
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
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include "Extent.h"

namespace Parfait {
template <typename T, size_t N>
Extent<T, N>::Extent() {}

template <typename T, size_t N>
Extent<T, N>::Extent(const T extent[N]) {
    static_assert(N == 3, "Expected N = 3 for Extent");
    lo[0] = extent[0];
    lo[1] = extent[1];
    lo[2] = extent[2];

    hi[0] = extent[3];
    hi[1] = extent[4];
    hi[2] = extent[5];
}

template <typename T, size_t N>
Extent<T, N>::Extent(const Point<T, N>& lo_in, const Point<T, N>& hi_in) : lo(lo_in), hi(hi_in) {}

template <typename T, size_t N>
bool Extent<T, N>::intersects(const Point<T, N>& p) const {
    for (unsigned int i = 0; i < N; i++) {
        if (p[i] < lo[i] || p[i] > hi[i]) {
            return false;
        }
    }
    return true;
}
template <typename T, size_t N>
bool Extent<T, N>::approxEqual(const Extent<T, N>& e) const {
    return lo.approxEqual(e.lo) and hi.approxEqual(e.hi);
}

template <typename T, size_t N>
bool Extent<T, N>::strictlyContains(const Extent<T, N>& e) const {
    for (unsigned int i = 0; i < N; i++) {
        if (e.lo[i] <= lo[i]) {
            return false;
        }
        if (e.hi[i] >= hi[i]) {
            return false;
        }
    }
    return true;
}

template <typename T, size_t N>
bool Extent<T, N>::intersects(const Extent<T, N>& box) const {
    for (unsigned int i = 0; i < N; i++) {
        if (box.hi[i] < lo[i]) {
            return false;
        }
        if (box.lo[i] > hi[i]) {
            return false;
        }
    }
    return true;
}

template <typename T, size_t N>
T Extent<T, N>::getLength_X() const {
    return hi[0] - lo[0];
}

template <typename T, size_t N>
T Extent<T, N>::getLength_Y() const {
    return hi[1] - lo[1];
}

template <typename T, size_t N>
T Extent<T, N>::getLength_Z() const {
    return hi[2] - lo[2];
}

template <typename T, size_t N>
std::string Extent<T, N>::to_string() const {
    std::string s = "lo: ";
    for (size_t i = 0; i < N; i++) s += std::to_string(lo[i]) + " ";
    s += ", hi: ";
    for (size_t i = 0; i < N; i++) s += std::to_string(hi[i]) + " ";
    return s;
}

template <typename T, size_t N>
Point<T, N> Extent<T, N>::center() const {
    Point<T, N> c;
    for (size_t i = 0; i < N; i++) {
        c[i] = 0.5 * (lo[i] + hi[i]);
    }
    return c;
}

template <typename T, size_t N>
void Extent<T, N>::resize(double scale) {
    static_assert(N == 3, "Expected N = 3 for Extent");
    scale *= 0.5;
    Point<T> centroid = center();
    double dx = scale * getLength_X();
    double dy = scale * getLength_Y();
    double dz = scale * getLength_Z();
    lo[0] = centroid[0] - dx;
    lo[1] = centroid[1] - dy;
    lo[2] = centroid[2] - dz;

    hi[0] = centroid[0] + dx;
    hi[1] = centroid[1] + dy;
    hi[2] = centroid[2] + dz;
}
template <typename T, size_t N>
Point<T, N> Extent<T, N>::clamp(Point<T, N> p) const {
    if (intersects(p)) return p;
    for (size_t i = 0; i < N; i++) {
        if (p[i] > hi[i])
            p[i] = hi[i];
        else if (p[i] < lo[i])
            p[i] = lo[i];
    }
    return p;
}

template <typename T, size_t N>
void Extent<T, N>::resize(double scaleX, double scaleY, double scaleZ) {
    static_assert(N == 3, "Expected N = 3 for Extent");
    double dx = 0.5 * scaleX * getLength_X();
    double dy = 0.5 * scaleY * getLength_Y();
    double dz = 0.5 * scaleZ * getLength_Z();
    Point<T> centroid = center();
    lo[0] = centroid[0] - dx;
    lo[1] = centroid[1] - dy;
    lo[2] = centroid[2] - dz;

    hi[0] = centroid[0] + dx;
    hi[1] = centroid[1] + dy;
    hi[2] = centroid[2] + dz;
}

template <typename T, size_t N>
void Extent<T, N>::makeIsotropic() {
    static_assert(N == 3, "Expected N = 3 for Extent");
    T x = getLength_X();
    T y = getLength_Y();
    T z = getLength_Z();
    T largest = std::max(x, y);
    largest = std::max(largest, z);
    largest /= 2;
    auto c = center();
    lo = c - Point<T>{largest, largest, largest};
    hi = c + Point<T>{largest, largest, largest};
}

template <typename T, size_t N>
Extent<T, N> Extent<T, N>::extentAroundPoint(const Point<T, N>& p, T tol) {
    Point<T, N> lo, hi;
    for (size_t i = 0; i < N; i++) {
        lo[i] = p[i] - tol;
    }
    for (size_t i = 0; i < N; i++) {
        hi[i] = p[i] + tol;
    }
    return Extent(lo, hi);
}

template <typename T, size_t N>
double Extent<T, N>::volume(const Extent<T, N>& domain) {
    return domain.getLength_X() * domain.getLength_Y() * domain.getLength_Z();
}

template <typename T, size_t N>
std::array<Point<T>, 8> Extent<T, N>::corners() const {
    static_assert(N == 3, "Expected N = 3 for Extent");
    std::array<Point<T>, 8> points;

    points[0] = {lo[0], lo[1], lo[2]};
    points[1] = {hi[0], lo[1], lo[2]};
    points[2] = {hi[0], hi[1], lo[2]};
    points[3] = {lo[0], hi[1], lo[2]};

    points[4] = {lo[0], lo[1], hi[2]};
    points[5] = {hi[0], lo[1], hi[2]};
    points[6] = {hi[0], hi[1], hi[2]};
    points[7] = {lo[0], hi[1], hi[2]};

    return points;
}

template <typename T, size_t N>
Extent<T, N> Extent<T, N>::intersection(const Extent<T, N>& b) const {
    Extent<T, N> e;
    for (size_t i = 0; i < N; i++) {
        e.lo[i] = std::max(b.lo[i], lo[i]);
        e.hi[i] = std::min(b.hi[i], hi[i]);
    }
    return e;
}
template <typename T, size_t N>
double Extent<T, N>::distance(const Point<T, N>& p) const {
    auto s = clamp(p);
    return Parfait::Point<T>::distance(p, s);
}
template <typename T, size_t N>
double Extent<T, N>::radius() const {
    return 0.5 * (hi - lo).magnitude();
}
template <typename T, size_t N>
int Extent<T, N>::longestDimension() const {
    int longest = 0;
    double max_dx = getLength_X();
    for (size_t i = 1; i < N; i++) {
        if (getLength(i) > max_dx) {
            max_dx = getLength(i);
            longest = i;
        }
    }
    return longest;
}
namespace {
    template <typename T>
    int sign(T val) {
        return (T(0) < val) - (val < T(0));
    }
}
template <typename T, size_t N>
bool Extent<T, N>::intersectsSegment(const Point<double, N>& a, const Point<double, N>& b) const {
    // Williams, A., Et al., "An Efficient and Robust Ray–Box Intersection Algorithm,"
    // Journal of Graphics, GPU, and Game Tools, Vol. 10, No. 1, 2005.
    auto direction = b - a;
    std::array<bool, 3> ray_sign;
    ray_sign[0] = (1.0 / direction[0] < 0.0);
    ray_sign[1] = (1.0 / direction[1] < 0.0);
    ray_sign[2] = (1.0 / direction[2] < 0.0);
    std::array<Parfait::Point<double>, 2> bounds{lo, hi};
    auto tmin = (bounds[ray_sign[0]][0] - a[0]) * (1.0 / direction[0]);
    auto tmax = (bounds[1 - ray_sign[0]][0] - a[0]) * (1.0 / direction[0]);
    auto tymin = (bounds[ray_sign[1]][1] - a[1]) * (1.0 / direction[1]);
    auto tymax = (bounds[1 - ray_sign[1]][1] - a[1]) * (1.0 / direction[1]);
    if ((tmin > tymax) || (tymin > tmax)) return false;
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;
    auto tzmin = (bounds[ray_sign[2]][2] - a[2]) * (1.0 / direction[2]);
    auto tzmax = (bounds[1 - ray_sign[2]][2] - a[2]) * (1.0 / direction[2]);
    if ((tmin > tzmax) || (tzmin > tmax)) return false;
    if (tzmin > tmin) tmin = tzmin;
    if (tzmax < tmax) tmax = tzmax;
    return ((tmin < 1.0) && (tmax > 0.0));
}
template <typename T, size_t N>
double Extent<T, N>::volume() const {
    return volume(*this);
}
template <typename T, size_t N>
T Extent<T, N>::getLength(int d) const {
    return hi[d] - lo[d];
}

}