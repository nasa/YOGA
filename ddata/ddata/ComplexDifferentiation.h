#pragma once

#include <complex>
#include <cmath>

#define COMPLEX_EQUALITY_OP(type, op)        \
    template <typename T>                    \
    bool operator op(type<T> a, type<T> b) { \
        return a.real() op b.real();         \
    }                                        \
    template <typename T>                    \
    bool operator op(type<T> a, T b) {       \
        return a.real() op b;                \
    }                                        \
    template <typename T>                    \
    bool operator op(T a, type<T> b) {       \
        return a op b.real();                \
    }

COMPLEX_EQUALITY_OP(std::complex, <)
COMPLEX_EQUALITY_OP(std::complex, <=)
COMPLEX_EQUALITY_OP(std::complex, >)
COMPLEX_EQUALITY_OP(std::complex, >=)

namespace Linearize {

COMPLEX_EQUALITY_OP(std::complex, <)
COMPLEX_EQUALITY_OP(std::complex, <=)
COMPLEX_EQUALITY_OP(std::complex, >)
COMPLEX_EQUALITY_OP(std::complex, >=)

template <typename T>
bool isnan(std::complex<T> a) {
    return (std::isnan(a.real()));
}
template <typename T>
bool isnan(T a) {
    return std::isnan(a);
}

template <typename T>
inline std::complex<T> abs(std::complex<T> a) {
    return a.real() > 0.0 ? a : -a;
}

template <typename T>
std::complex<T> min(std::complex<T> a, std::complex<T> b) {
    return a.real() < b.real() ? a : b;
}
template <typename T>
std::complex<T> min(T a, std::complex<T> b) {
    return a < b.real() ? a : b;
}
template <typename T>
std::complex<T> min(std::complex<T> a, T b) {
    return a.real() < b ? a : b;
}

template <typename T>
std::complex<T> max(std::complex<T> a, std::complex<T> b) {
    return a.real() > b.real() ? a : b;
}

template <typename T>
std::complex<T> max(T a, std::complex<T> b) {
    return a > b.real() ? a : b;
}

template <typename T>
std::complex<T> max(std::complex<T> a, T b) {
    return a.real() > b ? a : b;
}

#if __clang__
template <typename T, typename T2, std::enable_if_t<std::is_arithmetic<T2>::value, int> = 0>
std::complex<T> pow(const std::complex<T>& a, const T2& b) {
    T phi = std::atan(a.imag() / a.real());
    return std::pow(std::abs(a), b) * std::complex<T>(std::cos(phi * b), std::sin(phi * b));
}
template <typename T>
std::complex<T> pow(const std::complex<T>& a, const std::complex<T>& b) {
    return std::pow(a, b);
}
template <typename T, typename T2, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
std::complex<T> pow(const T& a, const std::complex<T2>& b) {
    return std::pow(a, b);
}
#else
using std::pow;
#endif

using std::acos;
using std::acosh;
using std::asin;
using std::asinh;
using std::atan;
using std::atanh;
using std::cos;
using std::cosh;
using std::exp;
using std::fabs;
using std::log;
using std::real;
using std::sin;
using std::sinh;
using std::sqrt;
using std::tan;
using std::tanh;
}
