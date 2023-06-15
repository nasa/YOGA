
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
#include <string>
#include "ParfaitMacros.h"

namespace Parfait {
template <typename T, size_t N = 3>
class Point {
  public:
    Point() = default;
    PARFAIT_DEVICE Point(const T& x, const T& y);
    PARFAIT_DEVICE Point(const T& x, const T& y, const T& z);
    PARFAIT_DEVICE Point(const T& x, const T& y, const T& z, const T& t);
    PARFAIT_DEVICE Point(const T& x, const T& y, const T& z, const T& u, const T& v, const T& t);
    PARFAIT_DEVICE Point(const T*);

    template <typename T2>
    PARFAIT_DEVICE Point(const Point<T2>& p);

    PARFAIT_DEVICE T* data() { return pos; }

    PARFAIT_DEVICE const T* data() const { return pos; }

    PARFAIT_INLINE T& operator[](int i) { return pos[i]; }

    PARFAIT_INLINE const T& operator[](int i) const { return pos[i]; }

    PARFAIT_DEVICE Point operator+(const Point& rhs) const;
    PARFAIT_DEVICE Point operator-(const Point& rhs) const;
    PARFAIT_DEVICE Point operator*(const T& a) const;
    PARFAIT_DEVICE Point operator/(const T& a) const;
    PARFAIT_DEVICE Point operator*=(const T& a);
    PARFAIT_DEVICE Point operator/=(const T& a);
    PARFAIT_DEVICE Point operator+=(const Point& rhs);
    PARFAIT_DEVICE Point operator-=(const Point& rhs);
    PARFAIT_DEVICE bool operator==(const Point& rhs) const;
    PARFAIT_DEVICE bool operator!=(const Point& rhs) const;
    PARFAIT_DEVICE bool operator<(const Point& rhs) const;
    PARFAIT_DEVICE bool operator>(const Point& rhs) const;
    PARFAIT_DEVICE bool approxEqual(const Point& rhs, T tol = 1.0e-14) const;
    PARFAIT_DEVICE void normalize();
    PARFAIT_DEVICE Point unit() const;
    PARFAIT_DEVICE T magnitude() const;
    PARFAIT_INLINE T magnitudeSquared() const;
    PARFAIT_DEVICE T distance(const Point& b) const;
    PARFAIT_DEVICE static T distance(const Point& a, const Point& b);
    PARFAIT_DEVICE static T dot(const Point& a, const Point& b);
    PARFAIT_DEVICE T dot(const Point& b) const;
    PARFAIT_DEVICE static T magnitude(const Point& a);
    PARFAIT_DEVICE static Point cross(const Point& a, const Point& b);
    PARFAIT_DEVICE Point cross(const Point& b) const;
    PARFAIT_DEVICE static Point normalize(const Point& a);

    // Methods not allowed on device
    std::string to_string() const;
    std::string to_string(const char* format_specifier) const;
    inline friend std::ostream& operator<<(std::ostream& os, const Point& t) {
        auto o = t.to_string("%e");
        os << o;
        return os;
    }

    Point(const std::array<T, N>& p);

  private:
    // This is explicitly a C-style array due to issues using std::array in CUDA.
    T pos[N];

    PARFAIT_DEVICE static bool containsNaNs(const Point<T, N>& p);
};
template <typename T>
PARFAIT_INLINE Parfait::Point<T> operator*(double scalar, const Parfait::Point<T>& p) {
    return p * scalar;
}
}

#include "Point.hpp"
