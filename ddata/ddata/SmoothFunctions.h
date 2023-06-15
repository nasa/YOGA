#pragma once
#include "ETDExpression.h"

namespace Linearize {
namespace Smooth {
    template <typename T>
    DDATA_DEVICE_FUNCTION T abs(T a, double eps = 1e-6) {
        if (a < 0.0) a = -a;
        return a >= eps ? a : 0.5 * (a * a / eps + eps);
    }
    template <typename T>
    DDATA_DEVICE_FUNCTION T min(const T& a, const T& b, double eps = 1e-6) {
        return 0.5 * (a + b - Linearize::Smooth::abs(T(a - b), eps));
    }
    template <typename T>
    DDATA_DEVICE_FUNCTION T max(const T& a, const T& b, double eps = 1e-6) {
        return -Linearize::Smooth::min(T(-a), T(-b), eps);
    }
    template <typename T>
    DDATA_DEVICE_FUNCTION T clamp(const T& x, const T& lower, const T& upper) {
        if (x < lower) return lower;
        if (x > upper) return upper;
        return x;
    }
    template <typename T>
    DDATA_DEVICE_FUNCTION T smooth_step(T x, double eps = 1e-6) {
        //
        // y = 0 if x < 0
        // y = 1 if x > 0
        // but smooth for x in [-eps, +eps]
        double edge0 = -eps;
        double edge1 = +eps;
        x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
        return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
    }
}
}
