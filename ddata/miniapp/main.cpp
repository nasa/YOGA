#ifdef USE_ETD
#include <ddata/ETD.h>
using namespace Linearize;
template <size_t N>
using AD_TYPE = ETD<N>;
#elif USE_ETDV
#include <ddata/ETDv.h>
using namespace Linearize;
template <size_t N>
using AD_TYPE = ETDv<N>;
#elif USE_DTD
#include <ddata/DTD.h>
using namespace Linearize;
template <size_t N>
using AD_TYPE = DTD<double>;
#elif USE_VTD
#include <ddata/VTD.h>
using namespace Linearize;
template <size_t N>
using AD_TYPE = VTD<double>;
#elif USE_SACADO
#include <Sacado_No_Kokkos.hpp>
template <size_t N>
using AD_TYPE = Sacado::ELRFad::SFad<double, N>;
#endif
#include "Functions.h"
#include <chrono>
#include <iostream>

#define PROFILE_RESIDUAL 1
#define PROFILE_AD 1
#define PROFILE_JACOBIAN 1

#define REPEAT_COUNT 10

int problem_size = static_cast<int>(PROBLEM_SIZE);

struct ADArray {
    using ADArrayType = Array<AD_TYPE<NumEqns()>>;
    auto& operator[](int i) { return v[i]; }
    inline void operator*=(double scalar) {
        for (int eqn = 0; eqn < NumEqns(); ++eqn) {
            v[eqn] *= scalar;
        }
    }
    inline void operator+=(const ADArrayType& rhs) {
        for (int eqn = 0; eqn < NumEqns(); ++eqn) {
            v[eqn] += rhs[eqn];
        }
    }
    ADArrayType v;
};

void checkValue(double expected, double actual) {
    auto diff = std::abs(actual - expected) / expected;
    if (diff > 1e-7) {
        printf("expected: %e actual: %e diff: %e\n", expected, actual, diff);
        throw std::runtime_error("incorrect result");
    }
}
auto calcAD(const Array<double> q, const std::array<double, 3>& n) {
    ADArray::ADArrayType dx{};
#ifdef USE_SACADO
    ADArray::ADArrayType q_ddt;
    for (int eqn = 0; eqn < NumEqns(); ++eqn) {
        q_ddt[eqn] = {NumEqns(), eqn, q[eqn]};
    }
#else
    using AD = AD_TYPE<NumEqns()>;
    auto q_ddt = AD::Identity(q);
#endif
    for (int i = 0; i < problem_size; ++i) {
        calcFlux(dx, q_ddt, n);
    }
    return dx;
}

auto calcJacobian(const Array<double> q, const std::array<double, 3>& n) {
    Array<Array<double>> dx{};
    for (int i = 0; i < problem_size; ++i) {
        calcJacobian(dx, q, n);
    }
    return dx;
}
int main() {
    Array<double> q = {1.0, 0.1, 0.2, 0.3, 3.0};
    std::array<double, 3> n = {0.33, 0.33, 0.33};
    using namespace std::chrono;
    Array<double> expected{};
    auto start = std::chrono::high_resolution_clock::now();
#ifdef PROFILE_RESIDUAL
    for (int i = 0; i < problem_size; ++i) {
        calcFlux(expected, q, n);
    }
#endif
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    std::cout << "(f) loop duration: " << duration.count() << " (ms)" << std::endl;

    std::cout << "flux: " << std::endl;
    for (int eqn = 0; eqn < NumEqns(); ++eqn) {
        std::cout << "eqn: " << eqn << ": " << expected[eqn] << std::endl;
    }

    auto expected_dx = calcJacobian(q, n);
#if PROFILE_JACOBIAN
    for (int repeat = 0; repeat < REPEAT_COUNT; repeat++) {
        start = std::chrono::high_resolution_clock::now();
        expected_dx = calcJacobian(q, n);
        end = std::chrono::high_resolution_clock::now();
        duration = duration_cast<milliseconds>(end - start);
        std::cout << "(Analytic) loop duration: " << duration.count() << " (ms)" << std::endl;
    }
#endif

    auto dx = calcAD(q, n);
#if PROFILE_AD
    for (int repeat = 0; repeat < REPEAT_COUNT; repeat++) {
        start = std::chrono::high_resolution_clock::now();
        dx = calcAD(q, n);
        end = std::chrono::high_resolution_clock::now();
        duration = duration_cast<milliseconds>(end - start);
        std::cout << "(AD) loop duration: " << duration.count() << " (ms)" << std::endl;
    }
#endif
    end = std::chrono::high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end - start);

    printf("check value\n");
    for (int eqn = 0; eqn < NumEqns(); ++eqn) {
#ifdef USE_SACADO
        checkValue(expected[eqn], dx[eqn].val());
#else
        checkValue(expected[eqn], dx[eqn].f());
#endif
    }
    printf("check dx\n");
    for (int eqn = 0; eqn < NumEqns(); ++eqn) {
        for (int dq = 0; dq < NumEqns(); ++dq) {
#ifdef USE_SACADO
            checkValue(expected_dx[eqn][dq], dx[eqn].dx(dq));
#else
            checkValue(expected_dx[eqn][dq], dx[eqn].dx_scalar(dq));
#endif
        }
    }

    return 0;
}
