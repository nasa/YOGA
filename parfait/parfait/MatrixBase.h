#pragma once
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <complex>
#include "Throw.h"
#include "ParfaitMacros.h"

namespace Parfait {
template <typename T>
struct MatrixTraits {};

template <typename Matrix>
class MatrixBase;

template <typename Matrix>
class MatrixTranspose : public MatrixBase<MatrixTranspose<Matrix>> {
  public:
    using value_type = typename Matrix::value_type;
    PARFAIT_DEVICE MatrixTranspose(const Matrix& matrix) : matrix(matrix) {}
    PARFAIT_DEVICE value_type operator()(int row, int col) const { return matrix(col, row); }
    PARFAIT_DEVICE int rows() const { return matrix.cols(); }
    PARFAIT_DEVICE int cols() const { return matrix.rows(); }
    const Matrix& matrix;
};
template <typename Matrix>
struct MatrixTraits<MatrixTranspose<Matrix>> {
    using value_type = typename MatrixTraits<Matrix>::value_type;
};

template <typename Matrix>
class SubMatrix : public MatrixBase<SubMatrix<Matrix>> {
  public:
    using value_type = typename Matrix::value_type;
    PARFAIT_DEVICE SubMatrix(int row_start, int col_start, int num_rows, int num_cols, const Matrix& matrix)
        : row_start(row_start), col_start(col_start), num_rows(num_rows), num_cols(num_cols), matrix(matrix) {}
    PARFAIT_DEVICE auto operator()(int row, int col) const { return matrix(row_start + row, col_start + col); }
    PARFAIT_DEVICE int rows() const { return num_rows; }
    PARFAIT_DEVICE int cols() const { return num_cols; }
    int row_start;
    int col_start;
    int num_rows;
    int num_cols;
    const Matrix& matrix;
};
template <typename Matrix>
struct MatrixTraits<SubMatrix<Matrix>> {
    using value_type = typename MatrixTraits<Matrix>::value_type;
};

template <typename Matrix>
class MatrixBase {
  public:
    using value_type = typename MatrixTraits<Matrix>::value_type;
    PARFAIT_DEVICE const Matrix& cast() const { return static_cast<const Matrix&>(*this); }
    PARFAIT_DEVICE Matrix& cast() { return static_cast<Matrix&>(*this); }
    PARFAIT_DEVICE operator value_type() {
        const auto& self = cast();
#ifndef __CUDA_ARCH__
        PARFAIT_ASSERT(self.rows() == 1 && self.cols() == 1, "Invalid scalar conversion.");
#endif
        return self(0, 0);
    }
    PARFAIT_DEVICE auto block(int row_start, int col_start, int num_rows, int num_cols) const {
        return SubMatrix<Matrix>(row_start, col_start, num_rows, num_cols, cast());
    }
    PARFAIT_DEVICE auto transpose() const { return MatrixTranspose<Matrix>(cast()); }
    template <typename RHS>
    PARFAIT_DEVICE auto dot(const MatrixBase<RHS>& rhs) const {
        typename Matrix::value_type result = (transpose() * rhs)(0, 0);
        return result;
    }
    PARFAIT_DEVICE auto trace() const {
        const auto& self = cast();
        typename Matrix::value_type t(0.0);
        for (int i = 0; i < self.rows(); i++) t += self(i, i);
        return t;
    }
    PARFAIT_DEVICE auto norm() const {
        const auto& self = cast();
        typename Matrix::value_type norm(0.0);
        for (int row = 0; row < self.rows(); ++row) {
            for (int col = 0; col < self.cols(); ++col) {
                norm += self(row, col) * self(row, col);
            }
        }
        using namespace std;
        norm = sqrt(norm);
        return norm;
    }
    auto absmax() const {
        const auto& self = cast();
        typename Matrix::value_type norm(0.0);
        for (int row = 0; row < self.rows(); ++row) {
            for (int col = 0; col < self.cols(); ++col) {
                norm = std::max(norm, std::abs(self(row, col)));
            }
        }
        return norm;
    }
    void print(double threshold = 0.0) const {
        auto& self = cast();
        using namespace std;
        for (int r = 0; r < self.rows(); ++r) {
            for (int c = 0; c < self.cols(); ++c) {
                auto v = self(r, c);
                if (abs(v) < threshold) v = 0.0;
                printf("%e ", real(v));
            }
            printf("\n");
        }
    }
};
}
