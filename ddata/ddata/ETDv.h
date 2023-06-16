#pragma once

#include "ETDOperators.h"
#include "ETDFunctions.h"
#include <array>
#include <complex>
#include <simd-math/simd.hpp>

namespace Linearize {
template <size_t n, typename T = double>
class ETDv : public ETDExpression<ETDv<n, T>> {
  public:
    using simd_t = simd::simd<T, simd::simd_abi::native>;
    using value_type = T;
    using derivative_type = simd_t;

    ~ETDv() = default;
    ETDv() = default;
    DDATA_DEVICE_FUNCTION ETDv(T value) : value(value), derivatives{0.0} {}
    DDATA_DEVICE_FUNCTION ETDv(T value, const std::array<T, n>& d) : value(value) {
        for (size_t i = 0; i < n; ++i) set_derivative(d[i], i);
    }
    DDATA_DEVICE_FUNCTION ETDv(T value, int derivative_index) : value(value), derivatives{0.0} {
        set_derivative(1.0, derivative_index);
    }
    template <typename E>
    DDATA_DEVICE_FUNCTION ETDv(const ETDExpression<E>& e) : value(e.cast().f()) {
        for (size_t i = 0; i < size_simd(); ++i) derivatives[i] = e.cast().dx(i);
    }

    // Accessors
    DDATA_DEVICE_FUNCTION value_type f() const { return value; }
    DDATA_DEVICE_FUNCTION derivative_type dx(int i) const { return derivatives[i]; }
    DDATA_DEVICE_FUNCTION value_type dx_scalar(int i) const {
        auto d_v = (T*)derivatives;
        return d_v[i];
    }
    DDATA_DEVICE_FUNCTION constexpr int size() const { return n; }

    DDATA_DEVICE_FUNCTION void set_value(const T& v) { value = v; }
    DDATA_DEVICE_FUNCTION void set_derivative(const T& v, int i) {
        auto d_v = (T*)derivatives;
        d_v[i] = v;
    }

    DDATA_DEVICE_FUNCTION ETDv& operator=(const T& v) {
        value = v;
        for (size_t i = 0; i < size_simd(); ++i) derivatives[i] = 0.0;
        return *this;
    }

    template <typename E>
    DDATA_DEVICE_FUNCTION ETDv& operator=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        for (size_t i = 0; i < size_simd(); ++i) derivatives[i] = et.dx(i);
        value = et.f();
        return *this;
    }

    template <typename E>
    DDATA_DEVICE_FUNCTION void operator+=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        value += et.f();
        for (size_t i = 0; i < size_simd(); ++i) {
            derivatives[i] += et.dx(i);
        }
    }
    DDATA_DEVICE_FUNCTION void operator+=(const T& scalar) { value += scalar; }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator-=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        value -= et.f();
        for (size_t i = 0; i < size_simd(); ++i) {
            derivatives[i] -= et.dx(i);
        }
    }
    DDATA_DEVICE_FUNCTION void operator-=(const T& scalar) { value -= scalar; }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator*=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        for (size_t i = 0; i < size_simd(); ++i) {
            derivatives[i] = value * et.dx(i) + et.f() * derivatives[i];
        }
        value *= et.f();
    }
    DDATA_DEVICE_FUNCTION void operator*=(const T& scalar) {
        value *= scalar;
        simd_t scalar_v(scalar);
        for (size_t i = 0; i < size_simd(); ++i) derivatives[i] *= scalar_v;
    }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator/=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        for (size_t i = 0; i < size_simd(); ++i) {
            derivatives[i] = (et.f() * derivatives[i] - value * et.dx(i)) / (et.f() * et.f());
        }
        value /= et.f();
    }
    DDATA_DEVICE_FUNCTION void operator/=(const T& scalar) {
        value /= scalar;
        simd_t scalar_v(scalar);
        for (size_t i = 0; i < size_simd(); ++i) derivatives[i] /= scalar_v;
    }

    static constexpr size_t size_simd() { return (n + simd_t::size() - 1) / simd_t::size(); }

    template <template <typename, size_t> class Array>
    DDATA_DEVICE_FUNCTION static Array<ETDv<n>, n> Identity(const Array<T, n>& values) {
        Array<ETDv<n>, n> d;
        for (size_t i = 0; i < n; ++i) {
            d[i] = ETDv<n>(std::real(values[i]), i);
        }
        return d;
    }

  private:
    value_type value;
    derivative_type derivatives[size_simd()];
};

template <typename T, size_t NumEqns, size_t NumVars, template <typename, size_t> class Array>
DDATA_DEVICE_FUNCTION Array<T, NumEqns * NumVars> ExtractBlockJacobianType(const Array<ETDv<NumVars>, NumEqns>& ddt) {
    Array<T, NumEqns * NumVars> block_jacobian;
    for (size_t i = 0; i < NumEqns; ++i)
        for (size_t j = 0; j < NumVars; ++j) block_jacobian[i * NumVars + j] = ddt[i].dx_scalar(j);
    return block_jacobian;
}

template <size_t NumEqns, size_t NumVars, template <typename, size_t> class Array>
DDATA_DEVICE_FUNCTION auto ExtractBlockJacobian(const Array<ETDv<NumVars>, NumEqns>& ddt) {
    return ExtractBlockJacobianType<double, NumEqns, NumVars>(ddt);
}
}
