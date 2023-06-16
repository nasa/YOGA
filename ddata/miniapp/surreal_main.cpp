#include <surreal/surreal_stack.h>
#include "Functions.h"
#include <chrono>
#include <iostream>

template <size_t N>
using AD_TYPE = SurrealS<N, double>;

int main() {
    AD_TYPE<problemSize()> a(1.0);
    AD_TYPE<problemSize()> b(2.0);
    AD_TYPE<problemSize()> c(0.0);
    using namespace std::chrono;
    int problem_size = static_cast<int>(1e8);
    double expected = 0.0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < problem_size; ++i) {
        expected += func<double>(1.0, 2.0);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    std::cout << "(f) loop duration: " << duration.count() << " (ms)" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < problem_size; ++i) {
        c += func(a, b);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end - start);

    std::cout << "(dx) loop duration: " << duration.count() << " (ms)" << std::endl;

    auto diff = std::abs(c.value() - expected) / expected;
    if (diff > 1e-7) {
        printf("expected: %e actual: %e diff: %e\n", expected, c.value(), diff);
        throw std::runtime_error("incorrect result");
    }

    return 0;
}