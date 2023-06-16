#pragma once
#include <array>

namespace Parfait {
namespace LinearAlgebra {
    inline std::array<double, 2> solve(const std::array<double, 4>& A, const std::array<double, 2>& b) {
        std::array<double, 2> x;
        double det = A[0] * A[3] - A[1] * A[2];
        double one_over_det = 1.0 / det;
        x[0] = one_over_det * (A[3] * b[0] - A[1] * b[1]);
        x[1] = one_over_det * (A[0] * b[1] - A[2] * b[0]);
        return x;
    }
}
}
