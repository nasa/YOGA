#pragma once

#include <array>
#include <vector>
#include <complex>
#include "ETDOperators.h"
#include "ETDFunctions.h"
#include "CustomAllocator.h"

namespace Linearize {
template <typename T>
class DTD : public ETDExpression<DTD<T>> {
  public:
    using value_type = T;
    using derivative_type = T;

    ~DTD() = default;
    DTD() : value(0.0) {}
    DDATA_DEVICE_FUNCTION DTD(int n, T value) : value(value), derivatives(n) {
        for (int i = 0; i < n; ++i) derivatives[i] = T(0.0);
    }
    template <size_t N>
    DDATA_DEVICE_FUNCTION DTD(T value, const std::array<T, N>& d) : value(value), derivatives(N) {
        for (size_t i = 0; i < N; ++i) derivatives[i] = d[i];
    }
    DDATA_DEVICE_FUNCTION DTD(int n, T value, int derivative_index) : value(value), derivatives(n) {
        for (int i = 0; i < static_cast<int>(n); ++i) derivatives[i] = i == derivative_index ? T(1.0) : T(0.0);
    }
    template <typename E>
    DDATA_DEVICE_FUNCTION DTD(const ETDExpression<E>& e) : value(e.cast().f()) {
        derivatives.resize(e.cast().size());
        for (int i = 0; i < e.cast().size(); ++i) derivatives[i] = e.cast().dx(i);
    }

    // Accessors
    DDATA_DEVICE_FUNCTION T f() const { return value; }
    DDATA_DEVICE_FUNCTION T dx(int i) const { return i < static_cast<int>(derivatives.size()) ? derivatives[i] : 0.0; }
    DDATA_DEVICE_FUNCTION T dx_scalar(int i) const { return dx(i); }
    DDATA_DEVICE_FUNCTION int size() const { return derivatives.size(); }

    DDATA_DEVICE_FUNCTION void set_value(const T& v) { value = v; }
    DDATA_DEVICE_FUNCTION void set_derivative(const T& d, int i) { derivatives[i] = d; }

    DDATA_DEVICE_FUNCTION DTD& operator=(const T& v) {
        value = v;
        for (int i = 0; i < size(); ++i) derivatives[i] = T(0.0);
        return *this;
    }

    template <typename E>
    DDATA_DEVICE_FUNCTION DTD& operator=(const ETDExpression<E>& e) {
        derivatives.resize(e.cast().size());
        const E& et = e.cast();
        for (size_t i = 0; i < size(); ++i) derivatives[i] = et.dx(i);
        value = et.f();
        return *this;
    }

    template <typename E>
    DDATA_DEVICE_FUNCTION void operator+=(const ETDExpression<E>& e) {
        derivatives.resize(e.cast().size());
        const E& et = e.cast();
        value += et.f();
        for (int i = 0; i < size(); ++i) {
            derivatives[i] += et.dx(i);
        }
    }
    DDATA_DEVICE_FUNCTION void operator+=(const T& scalar) { value += scalar; }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator-=(const ETDExpression<E>& e) {
        derivatives.resize(e.cast().size());
        const E& et = e.cast();
        value -= et.f();
        for (int i = 0; i < size(); ++i) {
            derivatives[i] -= et.dx(i);
        }
    }
    DDATA_DEVICE_FUNCTION void operator-=(const T& scalar) { value -= scalar; }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator*=(const ETDExpression<E>& e) {
        derivatives.resize(e.cast().size());
        const E& et = e.cast();
        for (int i = 0; i < size(); ++i) {
            derivatives[i] = value * et.dx(i) + et.f() * derivatives[i];
        }
        value *= et.f();
    }
    DDATA_DEVICE_FUNCTION void operator*=(const T& scalar) {
        value *= scalar;
        for (int i = 0; i < size(); ++i) {
            derivatives[i] *= scalar;
        }
    }
    template <typename E>
    DDATA_DEVICE_FUNCTION void operator/=(const ETDExpression<E>& e) {
        derivatives.resize(e.cast().size());
        const E& et = e.cast();
        for (int i = 0; i < size(); ++i) {
            derivatives[i] = (et.f() * derivatives[i] - value * et.dx(i)) / (et.f() * et.f());
        }
        value /= et.f();
    }
    DDATA_DEVICE_FUNCTION void operator/=(const T& scalar) {
        value /= scalar;
        for (int i = 0; i < size(); ++i) {
            derivatives[i] /= scalar;
        }
    }

    template <size_t n, template <typename, size_t> class Array>
    DDATA_DEVICE_FUNCTION static Array<DTD<T>, n> Identity(const Array<T, n>& values) {
        Array<DTD<T>, n> d;
        for (size_t i = 0; i < n; ++i) {
            d[i] = DTD<T>(n, std::real(values[i]), i);
        }
        return d;
    }

  private:
    T value;
    std::vector<T, MemoryPool<T>> derivatives;
};

template <typename T, size_t NumEqns, size_t NumVars, template <typename, size_t> class Array>
DDATA_DEVICE_FUNCTION Array<T, NumEqns * NumVars> ExtractBlockJacobianType(const Array<DTD<T>, NumEqns>& ddt) {
    Array<T, NumEqns * NumVars> block_jacobian;
    for (size_t i = 0; i < NumEqns; ++i)
        for (size_t j = 0; j < NumVars; ++j) block_jacobian[i * NumVars + j] = ddt[i].dx(j);
    return block_jacobian;
}

template <size_t NumEqns, size_t NumVars, template <typename, size_t> class Array>
DDATA_DEVICE_FUNCTION auto ExtractBlockJacobian(const Array<DTD<double>, NumEqns>& ddt) {
    return ExtractBlockJacobianType<double, NumEqns, NumVars>(ddt);
}
}
