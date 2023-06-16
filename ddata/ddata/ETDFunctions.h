#pragma once

#include "ETDExpression.h"
#include <utility>
#include <cmath>
#include <complex>

namespace Linearize {

/*
 * gcc and clang have different implementations of std::pow for complex, so we reimplement the gcc version for
 * consistency.  Because of these overloads, the pow function is always called as Linearize::pow to avoid
 * an ambiguous overload.
 */
#if __clang__
template <typename T,
          typename T2,
          std::enable_if_t<std::is_arithmetic<T>::value, int> = 0,
          std::enable_if_t<std::is_arithmetic<T2>::value, int> = 0>
DDATA_DEVICE_FUNCTION T pow(const T& a, const T2& b) {
    return std::pow(a, b);
}
#else
using std::pow;
#endif

// Include overloads to STL
using std::acos;
using std::asin;
using std::cos;
using std::exp;
using std::fabs;
using std::log;
using std::max;
using std::min;
using std::sin;
using std::sqrt;
using std::tanh;

DDATA_DEVICE_FUNCTION inline double real(double v) { return v; }
inline double real(const std::complex<double>& v) { return v.real(); }

template <typename E>
DDATA_DEVICE_FUNCTION typename E::value_type real(const ETDExpression<E>& e) {
    return real(e.cast().f());
}

#define ETD_UNARY_FUNCTION(function, derivative)                                                       \
    template <typename E>                                                                              \
    class ETD_##function : public ETDExpression<ETD_##function<E>> {                                   \
      public:                                                                                          \
        DDATA_DEVICE_FUNCTION ETD_##function(const E& e) : e(e), deriv_multiplier(calcDerivative()) {} \
        ETD_NO_COPY_CONSTRUCTOR(ETD_##function)                                                        \
        using value_type = typename E::value_type;                                                     \
        using derivative_type = typename E::derivative_type;                                           \
        const E& e;                                                                                    \
        const value_type deriv_multiplier;                                                             \
        DDATA_DEVICE_FUNCTION value_type f() const {                                                   \
            using std::function;                                                                       \
            return function(e.f());                                                                    \
        }                                                                                              \
        DDATA_DEVICE_FUNCTION derivative_type dx(int i) const { return deriv_multiplier * e.dx(i); }   \
        DDATA_DEVICE_FUNCTION int size() const { return e.size(); }                                    \
                                                                                                       \
      private:                                                                                         \
        DDATA_DEVICE_FUNCTION auto calcDerivative() const { return derivative; }                       \
    };                                                                                                 \
    template <typename E>                                                                              \
    DDATA_DEVICE_FUNCTION auto function(const ETDExpression<E>& e) {                                   \
        return ETD_##function<E>(e.cast());                                                            \
    }
ETD_UNARY_FUNCTION(abs, e.f() > 0.0 ? 1.0 : -1.0)
ETD_UNARY_FUNCTION(exp, exp(e.f()))
ETD_UNARY_FUNCTION(log, 1.0 / e.f())
ETD_UNARY_FUNCTION(tanh, 1.0 - Linearize::pow(tanh(e.f()), 2))
ETD_UNARY_FUNCTION(sin, cos(e.f()))
ETD_UNARY_FUNCTION(cos, -sin(e.f()))
ETD_UNARY_FUNCTION(asin, 1.0 / sqrt(1.0 - e.f() * e.f()))
ETD_UNARY_FUNCTION(acos, -1.0 / sqrt(1.0 - e.f() * e.f()))
ETD_UNARY_FUNCTION(sqrt, e.f() > 0.0 ? 0.5 / sqrt(e.f()) : 0.0)

#define ETD_BINARY_FUNCTION(function, value, derivative)                                            \
    template <typename E1, typename E2>                                                             \
    class ETD_##function : public ETDExpression<ETD_##function<E1, E2>> {                           \
      public:                                                                                       \
        DDATA_DEVICE_FUNCTION ETD_##function(const E1& e_1, const E2& e_2) : e1(e_1), e2(e_2) {     \
            ETD_TYPES_COMPATIBLE(E1, E2);                                                           \
        }                                                                                           \
        ETD_NO_COPY_CONSTRUCTOR(ETD_##function)                                                     \
        using value_type = typename E1::value_type;                                                 \
        using derivative_type = typename E1::derivative_type;                                       \
        const E1& e1;                                                                               \
        const E2& e2;                                                                               \
        DDATA_DEVICE_FUNCTION value_type f() const { return value; }                                \
        DDATA_DEVICE_FUNCTION derivative_type dx(int i) const { return derivative; }                \
        DDATA_DEVICE_FUNCTION int size() const { return std::max(e1.size(), e2.size()); }           \
    };                                                                                              \
    template <typename E1, typename E2>                                                             \
    DDATA_DEVICE_FUNCTION auto function(const ETDExpression<E1>& e1, const ETDExpression<E2>& e2) { \
        return ETD_##function<E1, E2>(e1.cast(), e2.cast());                                        \
    }
ETD_BINARY_FUNCTION(max, e1.f() > e2.f() ? e1.f() : e2.f(), e1.f() > e2.f() ? e1.dx(i) : e2.dx(i))
ETD_BINARY_FUNCTION(min, e1.f() < e2.f() ? e1.f() : e2.f(), e1.f() < e2.f() ? e1.dx(i) : e2.dx(i))

#define ETD_BINARY_SCALAR_FUNCTION(function, name, value, derivative)                               \
    template <typename E>                                                                           \
    class ETD_##name : public ETDExpression<ETD_##name<E>> {                                        \
      public:                                                                                       \
        using value_type = typename E::value_type;                                                  \
        using derivative_type = typename E::derivative_type;                                        \
        DDATA_DEVICE_FUNCTION ETD_##name(value_type scalar, const E& e) : scalar(scalar), e(e) {}   \
        ETD_NO_COPY_CONSTRUCTOR(ETD_##name)                                                         \
        const value_type scalar;                                                                    \
        const E& e;                                                                                 \
        DDATA_DEVICE_FUNCTION value_type f() const { return value; }                                \
        DDATA_DEVICE_FUNCTION derivative_type dx(int i) const { return derivative; }                \
        DDATA_DEVICE_FUNCTION int size() const { return e.size(); }                                 \
    };                                                                                              \
    template <typename E>                                                                           \
    DDATA_DEVICE_FUNCTION auto function(typename E::value_type scalar, const ETDExpression<E>& e) { \
        return ETD_##name<E>(scalar, e.cast());                                                     \
    }                                                                                               \
    template <typename E>                                                                           \
    DDATA_DEVICE_FUNCTION auto function(const ETDExpression<E>& e, typename E::value_type scalar) { \
        return ETD_##name<E>(scalar, e.cast());                                                     \
    }
ETD_BINARY_SCALAR_FUNCTION(max, ScalarMax, scalar > e.f() ? scalar : e.f(), scalar > e.f() ? value_type(0.0) : e.dx(i))
ETD_BINARY_SCALAR_FUNCTION(min, ScalarMin, scalar < e.f() ? scalar : e.f(), scalar < e.f() ? value_type(0.0) : e.dx(i))

template <typename E1, typename E2>
class ETD_Binary_Pow : public ETDExpression<ETD_Binary_Pow<E1, E2>> {
  public:
    DDATA_DEVICE_FUNCTION ETD_Binary_Pow(const E1& e_1, const E2& e_2)
        : e1(e_1),
          e2(e_2),
          value(Linearize::pow(e1.f(), e2.f())),
          deriv1(e2.f() * value / e1.f()),
          deriv2(log(e1.f()) * value) {
        ETD_TYPES_COMPATIBLE(E1, E2);
    }
    ETD_NO_COPY_CONSTRUCTOR(ETD_Binary_Pow)
    using value_type = typename E1::value_type;
    using derivative_type = typename E1::derivative_type;
    const E1& e1;
    const E2& e2;
    const value_type value;
    const value_type deriv1;
    const value_type deriv2;

    DDATA_DEVICE_FUNCTION value_type f() const { return value; }
    DDATA_DEVICE_FUNCTION derivative_type dx(int i) const { return deriv1 * e1.dx(i) + deriv2 * e2.dx(i); }
    DDATA_DEVICE_FUNCTION int size() const { return std::max(e1.size(), e2.size()); }
};
template <typename E1, typename E2>
DDATA_DEVICE_FUNCTION auto pow(const ETDExpression<E1>& e1, const ETDExpression<E2>& e2) {
    return ETD_Binary_Pow<E1, E2>(e1.cast(), e2.cast());
}

template <typename E>
class ETD_Binary_Pow_Base : public ETDExpression<ETD_Binary_Pow_Base<E>> {
  public:
    using value_type = typename E::value_type;
    using derivative_type = typename E::derivative_type;
    DDATA_DEVICE_FUNCTION ETD_Binary_Pow_Base(const E& e, value_type scalar)
        : e(e), exponent(scalar), value(Linearize::pow(e.f(), exponent)), deriv(calcDeriv()) {}
    ETD_NO_COPY_CONSTRUCTOR(ETD_Binary_Pow_Base)
    const E& e;
    const value_type exponent;
    const value_type value;
    const value_type deriv;

    DDATA_DEVICE_FUNCTION value_type f() const { return value; }
    DDATA_DEVICE_FUNCTION derivative_type dx(int i) const { return deriv * e.dx(i); }
    DDATA_DEVICE_FUNCTION int size() const { return e.size(); }

    DDATA_DEVICE_FUNCTION value_type calcDeriv() const { return exponent * Linearize::pow(e.f(), exponent - 1.0); }
};
template <typename E>
DDATA_DEVICE_FUNCTION auto pow(const ETDExpression<E>& e, typename E::value_type exponent) {
    return ETD_Binary_Pow_Base<E>(e.cast(), exponent);
}

template <typename E>
class ETD_Binary_Pow_Exponent : public ETDExpression<ETD_Binary_Pow_Exponent<E>> {
  public:
    using value_type = typename E::value_type;
    using derivative_type = typename E::derivative_type;
    DDATA_DEVICE_FUNCTION ETD_Binary_Pow_Exponent(value_type base, const E& e)
        : base(base), e(e), value(Linearize::pow(base, e.f())), deriv(calcDeriv()) {}
    ETD_NO_COPY_CONSTRUCTOR(ETD_Binary_Pow_Exponent)
    const value_type base;
    const E& e;
    const value_type value;
    const value_type deriv;

    DDATA_DEVICE_FUNCTION value_type f() const { return value; }
    DDATA_DEVICE_FUNCTION derivative_type dx(int i) const { return deriv * e.dx(i); }
    DDATA_DEVICE_FUNCTION int size() const { return e.size(); }

    value_type calcDeriv() const { return value * log(base); }
};
template <typename E>
DDATA_DEVICE_FUNCTION auto pow(typename E::value_type base, const ETDExpression<E>& e) {
    return ETD_Binary_Pow_Exponent<E>(base, e.cast());
}

// Special care is taken with std::abs, because it means something different for std::complex
template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
DDATA_DEVICE_FUNCTION T abs(const T& a) {
    return std::abs(a);
}
}