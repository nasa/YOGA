#pragma once

#include "ETDOperators.h"
#include "ETDFunctions.h"
#include <array>
#include <complex>

namespace Linearize {
template <size_t n, typename T = double>
class ETD : public ETDExpression<ETD<n, T>> {
  public:
    using value_type = T;
    using derivative_type = T;

    ~ETD() = default;
    ETD() = default;
    DDATA_DEVICE_FUNCTION ETD(T value) : value(value) {
        for (size_t i = 0; i < n; ++i) derivatives[i] = T(0.0);
    }
    DDATA_DEVICE_FUNCTION ETD(T value, const std::array<T, n>& d) : value(value) {
        for (size_t i = 0; i < n; ++i) derivatives[i] = d[i];
    }
    DDATA_DEVICE_FUNCTION ETD(T value, int derivative_index) : value(value) {
        for (int i = 0; i < static_cast<int>(n); ++i) derivatives[i] = i == derivative_index ? T(1.0) : T(0.0);
    }
    template <typename E>
    DDATA_DEVICE_FUNCTION ETD(const ETDExpression<E>& e) : value(e.cast().f()) {
        for (size_t i = 0; i < n; ++i) derivatives[i] = e.cast().dx(i);
    }

    // Accessors
    DDATA_DEVICE_FUNCTION T f() const { return value; }
    DDATA_DEVICE_FUNCTION T dx(int i) const { return derivatives[i]; }
    DDATA_DEVICE_FUNCTION T dx_scalar(int i) const { return derivatives[i]; }
    DDATA_DEVICE_FUNCTION constexpr int size() const { return n; }

    DDATA_DEVICE_FUNCTION void set_value(const T& v) { value = v; }
    DDATA_DEVICE_FUNCTION void set_derivative(const T& d, int i) { derivatives[i] = d; }

    DDATA_DEVICE_FUNCTION ETD& operator=(const T& v) {
        value = v;
        for (size_t i = 0; i < n; ++i) derivatives[i] = T(0.0);
        return *this;
    }

    template <typename E>
    DDATA_DEVICE_FUNCTION ETD& operator=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        for (size_t i = 0; i < n; ++i) derivatives[i] = et.dx(i);
        value = et.f();
        return *this;
    }

    template <typename E>
    DDATA_DEVICE_FUNCTION void operator+=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        value += et.f();
        for (size_t i = 0; i < n; ++i) {
            derivatives[i] += et.dx(i);
        }
    }
    DDATA_DEVICE_FUNCTION void operator+=(const T& scalar) { value += scalar; }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator-=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        value -= et.f();
        for (size_t i = 0; i < n; ++i) {
            derivatives[i] -= et.dx(i);
        }
    }
    DDATA_DEVICE_FUNCTION void operator-=(const T& scalar) { value -= scalar; }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator*=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        for (size_t i = 0; i < n; ++i) {
            derivatives[i] = value * et.dx(i) + et.f() * derivatives[i];
        }
        value *= et.f();
    }
    DDATA_DEVICE_FUNCTION void operator*=(const T& scalar) {
        value *= scalar;
        for (size_t i = 0; i < n; ++i) {
            derivatives[i] *= scalar;
        }
    }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator/=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        for (size_t i = 0; i < n; ++i) {
            derivatives[i] = (et.f() * derivatives[i] - value * et.dx(i)) / (et.f() * et.f());
        }
        value /= et.f();
    }
    DDATA_DEVICE_FUNCTION void operator/=(const T& scalar) {
        value /= scalar;
        for (size_t i = 0; i < n; ++i) {
            derivatives[i] /= scalar;
        }
    }

    template <template <typename, size_t> class Array>
    DDATA_DEVICE_FUNCTION static Array<ETD<n>, n> Identity(const Array<T, n>& values) {
        Array<ETD<n>, n> d;
        for (size_t i = 0; i < n; ++i) {
            d[i] = ETD<n>(std::real(values[i]), i);
        }
        return d;
    }

  private:
    T value;
    T derivatives[n];
};

template <typename T, size_t NumEqns, size_t NumVars, template <typename, size_t> class Array>
DDATA_DEVICE_FUNCTION Array<T, NumEqns * NumVars> ExtractBlockJacobianType(const Array<ETD<NumVars>, NumEqns>& ddt) {
    Array<T, NumEqns * NumVars> block_jacobian;
    for (size_t i = 0; i < NumEqns; ++i)
        for (size_t j = 0; j < NumVars; ++j) block_jacobian[i * NumVars + j] = ddt[i].dx(j);
    return block_jacobian;
}

template <size_t NumEqns, size_t NumVars, template <typename, size_t> class Array>
DDATA_DEVICE_FUNCTION auto ExtractBlockJacobian(const Array<ETD<NumVars>, NumEqns>& ddt) {
    return ExtractBlockJacobianType<double, NumEqns, NumVars>(ddt);
}
}
