
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
#include <assert.h>
#include <cmath>
#include <ostream>
#include "Point.h"

namespace Parfait {

template <typename T, size_t N>
template <typename T2>
PARFAIT_INLINE Point<T, N>::Point(const Point<T2>& p) : Point(p[0], p[1], p[2]) {
    static_assert(N == 3, "Trying to make N dimensional point from 3 values");
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N>::Point(const T& a, const T& b) {
    static_assert(N == 2, "Trying to make N dimensional point from 2 values");
    pos[0] = a;
    pos[1] = b;
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N>::Point(const T& a, const T& b, const T& c) {
    static_assert(N == 3, "Trying to make N dimensional point from 3 values");
    pos[0] = a;
    pos[1] = b;
    pos[2] = c;
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N>::Point(const T& a, const T& b, const T& c, const T& d) {
    static_assert(N == 4, "Trying to make N dimensional point from 4 values");
    pos[0] = a;
    pos[1] = b;
    pos[2] = c;
    pos[3] = d;
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N>::Point(const T& a, const T& b, const T& c, const T& d, const T& e, const T& f) {
    static_assert(N == 6, "Trying to make N dimensional point from 4 values");
    pos[0] = a;
    pos[1] = b;
    pos[2] = c;
    pos[3] = d;
    pos[4] = e;
    pos[5] = f;
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N>::Point(const T* p) {
    for (size_t i = 0; i < N; i++) {
        pos[i] = p[i];
    }
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::operator+(const Point<T, N>& rhs) const {
    static_assert(N == 3, "operator+ requires N=3");
    return Point(pos[0] + rhs.pos[0], pos[1] + rhs.pos[1], pos[2] + rhs.pos[2]);
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::operator-(const Point<T, N>& rhs) const {
    static_assert(N == 3, "operator- requires N=3");
    return Point<T, N>(pos[0] - rhs.pos[0], pos[1] - rhs.pos[1], pos[2] - rhs.pos[2]);
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::operator*(const T& a) const {
    static_assert(N == 3, "operator* requires N=3");
    return Point<T, N>(a * pos[0], a * pos[1], a * pos[2]);
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::operator/(const T& a) const {
    static_assert(N == 3, "operator/ requires N=3");
    T ooa = 1.0 / a;
    return Point<T, N>(ooa * pos[0], ooa * pos[1], ooa * pos[2]);
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::operator*=(const T& a) {
    for (size_t i = 0; i < N; i++) {
        pos[i] *= a;
    }
    return *this;
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::operator/=(const T& a) {
    T b = 1.0 / a;
    return *this *= b;
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::operator+=(const Point<T, N>& rhs) {
    for (size_t i = 0; i < N; i++) {
        pos[i] += rhs.pos[i];
    }
    return *this;
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::operator-=(const Point<T, N>& rhs) {
    for (size_t i = 0; i < N; i++) {
        pos[i] -= rhs.pos[i];
    }
    return *this;
}

template <typename T, size_t N>
PARFAIT_INLINE bool Point<T, N>::operator==(const Point<T, N>& rhs) const {
    for (size_t i = 0; i < N; i++) {
        if (pos[i] != rhs.pos[i]) return false;
    }
    return true;
}

template <typename T, size_t N>
PARFAIT_INLINE bool Point<T, N>::operator!=(const Point<T, N>& rhs) const {
    return not(*this == rhs);
}

template <typename T, size_t N>
PARFAIT_INLINE bool Point<T, N>::operator<(const Point<T, N>& rhs) const {
    for (size_t i = 0; i < N; i++) {
        if (pos[i] < rhs.pos[i])
            return true;
        else if (pos[i] > rhs.pos[i])
            return false;
    }
    return false;
}

template <typename T, size_t N>
PARFAIT_INLINE bool Point<T, N>::operator>(const Point<T, N>& rhs) const {
    return (not(*this < rhs)) and (not(*this == rhs));
}

template <typename T, size_t N>
PARFAIT_INLINE bool Point<T, N>::approxEqual(const Point<T, N>& rhs, T tol) const {
    for (size_t i = 0; i < N; i++) {
        if (pos[i] + tol < rhs.pos[i] or pos[i] - tol > rhs.pos[i]) return false;
    }
    if (containsNaNs(*this) or containsNaNs(rhs)) return false;
    return true;
}

template <typename T, size_t N>
PARFAIT_INLINE bool Point<T, N>::containsNaNs(const Point<T, N>& p) {
    for (size_t i = 0; i < N; i++) {
        if (std::isnan(p[i])) {
            return true;
        }
    }
    return false;
}

template <typename T, size_t N>
PARFAIT_INLINE T Point<T, N>::distance(const Point<T, N>& a, const Point<T, N>& b) {
    using namespace std;
    T s = (b.pos[0] - a.pos[0]) * (b.pos[0] - a.pos[0]);
    for (size_t i = 1; i < N; i++) {
        s += (b.pos[i] - a.pos[i]) * (b.pos[i] - a.pos[i]);
    }
    return sqrt(s);
}

template <typename T, size_t N>
PARFAIT_INLINE T Point<T, N>::distance(const Point& b) const {
    return distance(*this, b);
}

template <typename T, size_t N>
PARFAIT_INLINE T Point<T, N>::dot(const Point<T, N>& a, const Point<T, N>& b) {
    T s = a.pos[0] * b.pos[0];
    for (size_t i = 1; i < N; i++) {
        s += a.pos[i] * b.pos[i];
    }
    return s;
}

template <typename T, size_t N>
PARFAIT_INLINE T Point<T, N>::dot(const Point<T, N>& b) const {
    return dot(*this, b);
}

template <typename T, size_t N>
PARFAIT_INLINE T Point<T, N>::magnitude() const {
    using namespace std;
    T s = pos[0] * pos[0];
    for (size_t i = 1; i < N; i++) {
        s += pos[i] * pos[i];
    }
    return sqrt(s);
}

template <typename T, size_t N>
PARFAIT_INLINE T Point<T, N>::magnitudeSquared() const {
    using namespace std;
    T s = pos[0] * pos[0];
    for (size_t i = 1; i < N; i++) {
        s += pos[i] * pos[i];
    }
    return s;
}

template <typename T, size_t N>
PARFAIT_INLINE T Point<T, N>::magnitude(const Point<T, N>& a) {
    return a.magnitude();
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::cross(const Point<T, N>& a, const Point<T, N>& b) {
    static_assert(N == 3, "Not implemented for N != 3");
    return Point(a[1] * b[2] - b[1] * a[2], b[0] * a[2] - a[0] * b[2], a[0] * b[1] - b[0] * a[1]);
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::cross(const Point<T, N>& b) const {
    return cross(*this, b);
}

template <typename T, size_t N>
PARFAIT_INLINE void Point<T, N>::normalize() {
    T mag = magnitude(*this);
    mag += 1.0e-200;
    *this /= mag;
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::unit() const {
    auto norm = *this;
    norm.normalize();
    return norm;
}

template <typename T, size_t N>
PARFAIT_INLINE Point<T, N> Point<T, N>::normalize(const Point<T, N>& a) {
    Point<T, N> b{a};
    b.normalize();
    return b;
}

template <typename T, size_t N>
std::string Point<T, N>::to_string() const {
    std::string output = std::to_string(pos[0]);
    for (size_t i = 1; i < N; ++i) output += " " + std::to_string(pos[i]);
    return output;
}

template <typename T, size_t N>
Point<T, N>::Point(const std::array<T, N>& p) : Point(p[0], p[1], p[2]) {
    static_assert(N == 3, "Trying to make N dimensional point from 3 values");
}

namespace impl_point {
    template <typename... Args>
    std::string string_format(const std::string& format, Args... args) {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;  // Extra space for '\0'
        if (size_s <= 0) {
            throw std::runtime_error("Error during formatting.");
        }
        auto size = static_cast<size_t>(size_s);
        std::vector<char> buf(size);
        std::snprintf(buf.data(), size, format.c_str(), args...);
        return std::string(buf.data(), buf.data() + size - 1);  // We don't want the '\0' inside
    }
}
template <typename T, size_t N>
std::string Point<T, N>::to_string(const char* format_specifier) const {
    std::string format = "";
    for (size_t i = 0; i < N - 1; i++) {
        format += std::string(format_specifier) + " ";
    }
    format += format_specifier;

    if (N == 1) {
        return impl_point::string_format(format, (T)pos[0]);
    }
    if (N == 2) {
        return impl_point::string_format(format, (T)pos[0], (T)pos[1]);
    }
    if (N == 3) {
        return impl_point::string_format(format, (T)pos[0], (T)pos[1], (T)pos[2]);
    }
    if (N == 4) {
        return impl_point::string_format(format, pos[0], pos[1], pos[2], pos[3]);
    }
    static_assert(N <= 4, "need to implement to_string(fmt) for Point dimension > 4");
    return "";
}

}
