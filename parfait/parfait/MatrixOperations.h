#pragma once
#include "MatrixBase.h"
#include "ParfaitMacros.h"
#include "Throw.h"

namespace Parfait {

#define PARFAIT_DENSE_MATRIX_BINARY_OP_TRAITS(op)                  \
    template <typename LHS, typename RHS>                          \
    struct MatrixTraits<op<LHS, RHS>> {                            \
        using value_type = typename MatrixTraits<LHS>::value_type; \
    };

template <typename LHS, typename RHS>
class MatrixSum : public MatrixBase<MatrixSum<LHS, RHS>> {
  public:
    PARFAIT_DEVICE MatrixSum(const LHS& lhs, const RHS& rhs) : lhs(lhs), rhs(rhs) {
        static_assert(std::is_same<typename LHS::value_type, typename RHS::value_type>::value,
                      "mismatch in matrix value types");
#ifndef __CUDA_ARCH__
        PARFAIT_ASSERT(lhs.rows() == rhs.rows(), "Matrix row size mismatch");
        PARFAIT_ASSERT(lhs.cols() == rhs.cols(), "Matrix column size mismatch");
#endif
    }
    using value_type = typename LHS::value_type;
    PARFAIT_DEVICE static constexpr bool isAliasSafe() { return LHS::isAliasSafe() and RHS::isAliasSafe(); }
    PARFAIT_DEVICE value_type operator()(int row, int col) const { return lhs(row, col) + rhs(row, col); }

    PARFAIT_DEVICE constexpr int rows() const { return lhs.rows(); }
    PARFAIT_DEVICE constexpr int cols() const { return lhs.cols(); }

    const LHS& lhs;
    const RHS& rhs;
};

PARFAIT_DENSE_MATRIX_BINARY_OP_TRAITS(MatrixSum)

template <typename LHS, typename RHS>
PARFAIT_DEVICE auto operator+(const MatrixBase<LHS>& lhs, const MatrixBase<RHS>& rhs) {
    return MatrixSum<LHS, RHS>(lhs.cast(), rhs.cast());
}

template <typename LHS, typename RHS>
PARFAIT_DEVICE MatrixBase<LHS>& operator+=(MatrixBase<LHS>& lhs, const MatrixBase<RHS>& rhs) {
    lhs.cast() = lhs + rhs;
    return lhs;
}

template <typename LHS, typename RHS>
class MatrixDifference : public MatrixBase<MatrixDifference<LHS, RHS>> {
  public:
    PARFAIT_DEVICE MatrixDifference(const LHS& lhs, const RHS& rhs) : lhs(lhs), rhs(rhs) {
        static_assert(std::is_same<typename LHS::value_type, typename RHS::value_type>::value,
                      "mismatch in matrix value types");
#ifndef __CUDA_ARCH__
        PARFAIT_ASSERT(lhs.rows() == rhs.rows(), "Matrix row size mismatch");
        PARFAIT_ASSERT(lhs.cols() == rhs.cols(), "Matrix column size mismatch");
#endif
    }
    using value_type = typename LHS::value_type;
    PARFAIT_DEVICE static constexpr bool isAliasSafe() { return LHS::isAliasSafe() and RHS::isAliasSafe(); }
    PARFAIT_DEVICE value_type operator()(int row, int col) const { return lhs(row, col) - rhs(row, col); }

    PARFAIT_DEVICE constexpr int rows() const { return lhs.rows(); }
    PARFAIT_DEVICE constexpr int cols() const { return lhs.cols(); }

    const LHS& lhs;
    const RHS& rhs;
};
PARFAIT_DENSE_MATRIX_BINARY_OP_TRAITS(MatrixDifference)

template <typename LHS, typename RHS>
PARFAIT_DEVICE auto operator-(const MatrixBase<LHS>& lhs, const MatrixBase<RHS>& rhs) {
    return MatrixDifference<LHS, RHS>(lhs.cast(), rhs.cast());
}

template <typename LHS, typename RHS>
PARFAIT_DEVICE MatrixBase<LHS>& operator-=(MatrixBase<LHS>& lhs, const MatrixBase<RHS>& rhs) {
    lhs.cast() = lhs - rhs;
    return lhs;
}

template <typename LHS, typename RHS>
class MatrixMultiply : public MatrixBase<MatrixMultiply<LHS, RHS>> {
  public:
    PARFAIT_DEVICE MatrixMultiply(const LHS& lhs, const RHS& rhs) : lhs(lhs), rhs(rhs) {
        static_assert(std::is_same<typename LHS::value_type, typename RHS::value_type>::value,
                      "mismatch in matrix value types");
#ifndef __CUDA_ARCH__
        PARFAIT_ASSERT(lhs.cols() == rhs.rows(), "Incompatible Matrix dimensions");
#endif
    }
    using value_type = typename LHS::value_type;
    PARFAIT_DEVICE static constexpr bool isAliasSafe() { return false; }
    PARFAIT_DEVICE value_type operator()(int row, int col) const {
        value_type sum = 0.0;
        for (int i = 0; i < lhs.cols(); ++i) sum += lhs(row, i) * rhs(i, col);
        return sum;
    }

    PARFAIT_DEVICE constexpr int rows() const { return lhs.rows(); }
    PARFAIT_DEVICE constexpr int cols() const { return rhs.cols(); }

    const LHS& lhs;
    const RHS& rhs;
};
PARFAIT_DENSE_MATRIX_BINARY_OP_TRAITS(MatrixMultiply)

template <typename LHS, typename RHS>
PARFAIT_DEVICE auto operator*(const MatrixBase<LHS>& lhs, const MatrixBase<RHS>& rhs) {
    return MatrixMultiply<LHS, RHS>(lhs.cast(), rhs.cast());
}

template <typename Matrix>
class MatrixScalarMultiply : public MatrixBase<MatrixScalarMultiply<Matrix>> {
  public:
    using value_type = typename Matrix::value_type;
    PARFAIT_DEVICE MatrixScalarMultiply(value_type scalar, const Matrix& matrix) : scalar(scalar), matrix(matrix) {}
    PARFAIT_DEVICE value_type operator()(int row, int col) const { return scalar * matrix(row, col); }
    PARFAIT_DEVICE static constexpr bool isAliasSafe() { return Matrix::isAliasSafe(); }
    PARFAIT_DEVICE constexpr int rows() const { return matrix.rows(); }
    PARFAIT_DEVICE constexpr int cols() const { return matrix.cols(); }

    const value_type scalar;
    const Matrix& matrix;
};

template <typename Matrix>
struct MatrixTraits<MatrixScalarMultiply<Matrix>> {
    using value_type = typename MatrixTraits<Matrix>::value_type;
};

template <typename RHS>
PARFAIT_DEVICE auto operator*(typename RHS::value_type scalar, const MatrixBase<RHS>& rhs) {
    return MatrixScalarMultiply<RHS>(scalar, rhs.cast());
}
template <typename LHS>
PARFAIT_DEVICE auto operator*(const MatrixBase<LHS>& lhs, typename LHS::value_type scalar) {
    return MatrixScalarMultiply<LHS>(scalar, lhs.cast());
}
template <typename LHS>
PARFAIT_DEVICE MatrixBase<LHS>& operator*=(MatrixBase<LHS>& lhs, typename LHS::value_type scalar) {
    lhs.cast() = lhs * scalar;
    return lhs;
}

template <typename LHS>
PARFAIT_DEVICE auto operator/(const MatrixBase<LHS>& lhs, typename LHS::value_type scalar) {
    return MatrixScalarMultiply<LHS>(1.0 / scalar, lhs.cast());
}
template <typename LHS>
PARFAIT_DEVICE MatrixBase<LHS>& operator/=(MatrixBase<LHS>& lhs, typename LHS::value_type scalar) {
    lhs.cast() = lhs / scalar;
    return lhs;
}

template <typename T, int Rows, int Columns>
class IdentityMatrix : public MatrixBase<IdentityMatrix<T, Rows, Columns>> {
  public:
    using value_type = T;
    PARFAIT_DEVICE IdentityMatrix(int rows, int columns) : _rows(rows), _cols(columns) {}
    PARFAIT_DEVICE IdentityMatrix(int dimension)
        : IdentityMatrix(Rows == Dynamic ? dimension : Rows, Columns == Dynamic ? dimension : Columns) {
        static_assert(Rows != Dynamic or Columns != Dynamic, "incomplete dimensions");
    }
    PARFAIT_DEVICE IdentityMatrix() : IdentityMatrix(Rows, Columns) {
        static_assert(Rows != Dynamic and Columns != Dynamic, "incomplete dimensions");
    }
    PARFAIT_DEVICE const value_type operator()(int row, int col) const { return row == col ? 1.0 : 0.0; }
    PARFAIT_DEVICE static constexpr bool isAliasSafe() { return true; }
    PARFAIT_DEVICE constexpr int rows() const { return _rows; }
    PARFAIT_DEVICE constexpr int cols() const { return _cols; }
    int _rows;
    int _cols;
};
template <typename T, int Rows, int Columns>
struct MatrixTraits<IdentityMatrix<T, Rows, Columns>> {
    using value_type = T;
};

template <typename T, int Rows, int Columns>
class ZeroMatrix : public MatrixBase<ZeroMatrix<T, Rows, Columns>> {
  public:
    using value_type = T;
    PARFAIT_DEVICE ZeroMatrix(int rows, int columns) : _rows(rows), _cols(columns) {}
    PARFAIT_DEVICE ZeroMatrix(int dimension)
        : ZeroMatrix(Rows == Dynamic ? dimension : Rows, Columns == Dynamic ? dimension : Columns) {
        static_assert(Rows != Dynamic or Columns != Dynamic, "incomplete dimensions");
    }
    PARFAIT_DEVICE ZeroMatrix() : ZeroMatrix(Rows, Columns) {
        static_assert(Rows != Dynamic and Columns != Dynamic, "incomplete dimensions");
    }
    PARFAIT_DEVICE static constexpr bool isAliasSafe() { return true; }
    PARFAIT_DEVICE const value_type operator()(int row, int col) const { return 0.0; }
    PARFAIT_DEVICE constexpr int rows() const { return _rows; }
    PARFAIT_DEVICE constexpr int cols() const { return _cols; }
    int _rows;
    int _cols;
};
template <typename T, int Rows, int Columns>
struct MatrixTraits<ZeroMatrix<T, Rows, Columns>> {
    using value_type = T;
};
}
