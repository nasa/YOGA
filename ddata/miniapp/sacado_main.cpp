#include <Sacado_No_Kokkos.hpp>
#include "Functions.h"
#include <chrono>
#include <iostream>

// template<size_t N>
// using AD_TYPE = Sacado::Fad::DFad<double>;
template <size_t N>
using AD_TYPE = Sacado::Fad::SFad<double, N>;

void checkValue(double expected, double actual) {
    auto diff = std::abs(actual - expected) / expected;
    if (diff > 1e-7) {
        printf("expected: %e actual: %e diff: %e\n", expected, actual, diff);
        throw std::runtime_error("incorrect result");
    }
}

int main() {
    AD_TYPE<problemSize()> a(2, 0, 1.0);
    AD_TYPE<problemSize()> b(2, 1, 2.0);
    AD_TYPE<problemSize()> c(0.0);
    //    Sacado::Rad::ADvar<double> a = 1.0;
    //    Sacado::Rad::ADvar<double> b = 2.0;
    //    Sacado::Rad::ADvar<double> c = 0.0;
    using namespace std::chrono;
    int problem_size = static_cast<int>(1e8);
    double expected = 0.0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < problem_size; ++i) {
        func<double>(expected, 1.0, 2.0);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    std::cout << "(f) loop duration: " << duration.count() << " (ms)" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < problem_size; ++i) {
        func(c, a, b);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end - start);

    std::cout << "(AD) loop duration: " << duration.count() << " (ms)" << std::endl;

    double expected_dx = 0.0;
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < problem_size; ++i) {
        expected_dx += 1.0 + std::cos(a.val()) + 0.5 / std::sqrt(a.val());
    }
    end = std::chrono::high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end - start);

    std::cout << "(Analytic) loop duration: " << duration.count() << " (ms)" << std::endl;

    printf("check value\n");
    checkValue(expected, c.val());
    printf("check dx\n");
    checkValue(expected_dx, c.dx(0));
    return 0;
}