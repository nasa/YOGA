#pragma once

#include <array>
#include <vector>
#include <complex>
#include "ETDOperators.h"
#include "ETDFunctions.h"
#include "CustomAllocator.h"
#include <simd-math/simd.hpp>

namespace Linearize {
template <typename T>
class VTD : public ETDExpression<VTD<T>> {
  public:
    using simd_t = simd::simd<T, simd::simd_abi::native>;
    using value_type = T;
    using derivative_type = simd_t;

    ~VTD() = default;
    VTD() : n_derivatives(0), value(0.0) {}
    DDATA_DEVICE_FUNCTION VTD(T value) : n_derivatives(0), value(value) {}
    DDATA_DEVICE_FUNCTION VTD(int n, T value)
        : n_derivatives(n), value(value), derivatives(calcDerivativeVectorSize(), simd_t(0.0)) {}
    template <size_t N>
    DDATA_DEVICE_FUNCTION VTD(T value, const std::array<T, N>& d)
        : n_derivatives(N), value(value), derivatives(calcDerivativeVectorSize()) {
        for (size_t i = 0; i < N; ++i) setDerivative(i, d[i]);
    }
    DDATA_DEVICE_FUNCTION VTD(int n, T value, int derivative_index)
        : n_derivatives(n), value(value), derivatives(calcDerivativeVectorSize(), simd_t(0.0)) {
        setDerivative(derivative_index, 1.0);
    }
    template <typename E>
    DDATA_DEVICE_FUNCTION VTD(const ETDExpression<E>& e)
        : n_derivatives(e.cast().size()), value(e.cast().f()), derivatives(calcDerivativeVectorSize()) {
        for (size_t i = 0; i < derivatives.size(); ++i) derivatives[i] = e.cast().dx(i);
    }

    // Accessors
    DDATA_DEVICE_FUNCTION value_type f() const { return value; }
    DDATA_DEVICE_FUNCTION derivative_type dx(int i) const { return i < (int)derivatives.size() ? derivatives[i] : 0.0; }
    DDATA_DEVICE_FUNCTION int size() const { return n_derivatives; }

    DDATA_DEVICE_FUNCTION value_type dx_scalar(int i) const {
        auto d_v = (T*)derivatives.data();
        return i < n_derivatives ? d_v[i] : 0.0;
    }

    DDATA_DEVICE_FUNCTION void set_value(const T& v) { value = v; }
    DDATA_DEVICE_FUNCTION void set_derivative(const T& d, int i) { setDerivative(i, d); }

    DDATA_DEVICE_FUNCTION VTD& operator=(const T& v) {
        value = v;
        for (auto& derivative : derivatives) derivative = simd_t(0.0);
        return *this;
    }

    template <typename E>
    DDATA_DEVICE_FUNCTION VTD& operator=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        n_derivatives = et.size();
        derivatives.resize(calcDerivativeVectorSize());
        for (size_t i = 0; i < derivatives.size(); ++i) derivatives[i] = et.dx(i);
        value = et.f();
        return *this;
    }

    template <typename E>
    DDATA_DEVICE_FUNCTION void operator+=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        n_derivatives = et.size();
        derivatives.resize(calcDerivativeVectorSize());
        value += et.f();
        for (size_t i = 0; i < derivatives.size(); ++i) {
            derivatives[i] += et.dx(i);
        }
    }
    DDATA_DEVICE_FUNCTION void operator+=(const T& scalar) { value += scalar; }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator-=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        n_derivatives = et.size();
        derivatives.resize(calcDerivativeVectorSize());
        value -= et.f();
        for (int i = 0; i < size(); ++i) {
            derivatives[i] -= et.dx(i);
        }
    }
    DDATA_DEVICE_FUNCTION void operator-=(const T& scalar) { value -= scalar; }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator*=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        n_derivatives = et.size();
        derivatives.resize(calcDerivativeVectorSize());
        for (size_t i = 0; i < derivatives.size(); ++i) {
            derivatives[i] = value * et.dx(i) + et.f() * derivatives[i];
        }
        value *= et.f();
    }
    DDATA_DEVICE_FUNCTION void operator*=(const T& scalar) {
        value *= scalar;
        simd_t scalar_v(scalar);
        for (auto& d : derivatives) d *= scalar_v;
    }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator/=(const ETDExpression<E>& e) {
        const E& et = e.cast();
        n_derivatives = et.size();
        derivatives.resize(calcDerivativeVectorSize());
        for (size_t i = 0; i < derivatives.size(); ++i) {
            derivatives[i] = (et.f() * derivatives[i] - value * et.dx(i)) / (et.f() * et.f());
        }
        value /= et.f();
    }
    DDATA_DEVICE_FUNCTION void operator/=(const T& scalar) {
        value /= scalar;
        simd_t scalar_v(scalar);
        for (auto& d : derivatives) d /= scalar_v;
    }

    template <size_t n, template <typename, size_t> class Array>
    DDATA_DEVICE_FUNCTION static Array<VTD<T>, n> Identity(const Array<T, n>& values) {
        Array<VTD<T>, n> d;
        for (size_t i = 0; i < n; ++i) {
            d[i] = VTD<T>(n, std::real(values[i]), i);
        }
        return d;
    }

  private:
    int n_derivatives;
    value_type value;
    std::vector<derivative_type, MemoryPool<derivative_type>> derivatives;

    int calcDerivativeVectorSize() const { return (n_derivatives + simd_t::size() - 1) / simd_t::size(); }
    void setDerivative(int i, const T& v) {
        auto d_v = (T*)derivatives.data();
        d_v[i] = v;
    }
};

template <typename T, size_t NumEqns, size_t NumVars, template <typename, size_t> class Array>
DDATA_DEVICE_FUNCTION Array<T, NumEqns * NumVars> ExtractBlockJacobianType(const Array<VTD<T>, NumEqns>& ddt) {
    Array<T, NumEqns * NumVars> block_jacobian;
    for (size_t i = 0; i < NumEqns; ++i)
        for (size_t j = 0; j < NumVars; ++j) block_jacobian[i * NumVars + j] = ddt[i].dx(j);
    return block_jacobian;
}

template <size_t NumEqns, size_t NumVars, template <typename, size_t> class Array>
DDATA_DEVICE_FUNCTION auto ExtractBlockJacobian(const Array<VTD<double>, NumEqns>& ddt) {
    return ExtractBlockJacobianType<double, NumEqns, NumVars>(ddt);
}
}
