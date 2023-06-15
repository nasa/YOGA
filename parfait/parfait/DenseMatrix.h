#pragma once
#include "MatrixBase.h"
#include "MatrixStorage.h"
#include "MatrixOperations.h"

namespace Parfait {

template <typename Matrix>
class MutableSubMatrix : public MatrixBase<MutableSubMatrix<Matrix>> {
  public:
    using value_type = typename Matrix::value_type;
    PARFAIT_DEVICE MutableSubMatrix(int row_start, int col_start, int num_rows, int num_cols, Matrix& matrix)
        : row_start(row_start), col_start(col_start), num_rows(num_rows), num_cols(num_cols), matrix(matrix) {}
    PARFAIT_DEVICE static constexpr bool isAliasSafe() { return true; }

    template <typename RHS>
    PARFAIT_DEVICE MutableSubMatrix& operator=(const MatrixBase<RHS>& rhs) {
#ifndef __CUDA_ARCH__
        PARFAIT_ASSERT(rows() == rhs.cast().rows(), "Matrix assignment rhs has wrong number of rows");
        PARFAIT_ASSERT(cols() == rhs.cast().cols(), "Matrix assignment rhs has wrong number of columns");
#endif
        if (RHS::isAliasSafe()) {
            Matrix::setValues(*this, rhs);
        } else {
            Matrix tmp = *this;
            Matrix::setValues(tmp, rhs);
            *this = tmp;
        }
        return *this;
    }
    PARFAIT_DEVICE value_type& operator()(int row, int col) { return matrix(row_start + row, col_start + col); }
    PARFAIT_DEVICE const value_type& operator()(int row, int col) const {
        return matrix(row_start + row, col_start + col);
    }
    PARFAIT_DEVICE int rows() const { return num_rows; }
    PARFAIT_DEVICE int cols() const { return num_cols; }
    int row_start;
    int col_start;
    int num_rows;
    int num_cols;
    Matrix& matrix;
};

namespace MatrixOrdering {
    struct RowMajor {};
    struct ColumnMajor {};
}

template <typename T,
          int Rows = Parfait::Dynamic,
          int Columns = Parfait::Dynamic,
          class Indexing = MatrixOrdering::RowMajor>
class DenseMatrix : public MatrixBase<DenseMatrix<T, Rows, Columns, Indexing>> {
  public:
    using value_type = T;
    PARFAIT_DEVICE static constexpr bool isAliasSafe() { return true; }

    PARFAIT_DEVICE DenseMatrix(int num_rows, int num_columns) : values(num_rows, num_columns) {}
    PARFAIT_DEVICE DenseMatrix() : DenseMatrix(0, 0) {}

    template <typename RHS>
    PARFAIT_DEVICE DenseMatrix(const MatrixBase<RHS>& rhs) : DenseMatrix(rhs.cast().rows(), rhs.cast().cols()) {
        setValues(*this, rhs);
    }
    PARFAIT_DEVICE DenseMatrix(const T* rhs) : values(Rows, Columns) {
        static_assert(Rows != Dynamic && Columns != Dynamic, "Pointer ctor required a fixed-sized DenseMatrix.");
        for (int i = 0; i < Rows * Columns; ++i) values.m[i] = rhs[i];
    }
    DenseMatrix(const std::initializer_list<T>& rhs) : values(rhs.size(), 1) { setValues(rhs); }
    DenseMatrix(const std::initializer_list<std::initializer_list<T>>& rhs)
        : values(rhs.size(), rhs.size() > 0 ? rhs.begin()->size() : 0) {
        setValues(rhs);
    }

    DenseMatrix& operator=(const std::initializer_list<std::initializer_list<T>>& rhs) {
        setValues(rhs);
        return *this;
    }
    DenseMatrix& operator=(const std::initializer_list<T>& rhs) {
        setValues(rhs);
        return *this;
    }
    template <typename RHS>
    PARFAIT_DEVICE DenseMatrix& operator=(const MatrixBase<RHS>& rhs) {
#ifndef __CUDA_ARCH__
        PARFAIT_ASSERT(rows() == rhs.cast().rows(), "Matrix assignment rhs has wrong number of rows");
        PARFAIT_ASSERT(cols() == rhs.cast().cols(), "Matrix assignment rhs has wrong number of columns");
#endif
        if (RHS::isAliasSafe()) {
            setValues(*this, rhs);
        } else {
            DenseMatrix tmp = *this;
            setValues(tmp, rhs);
            *this = tmp;
        }
        return *this;
    }

    PARFAIT_DEVICE T& operator()(int row, int column) { return values.m[getIndex(row, column, Indexing{})]; }
    PARFAIT_DEVICE const T& operator()(int row, int column) const {
        return values.m[getIndex(row, column, Indexing{})];
    }
    PARFAIT_DEVICE T& operator[](int i) { return values.m[i]; }
    PARFAIT_DEVICE const T& operator[](int i) const { return values.m[i]; }

    PARFAIT_DEVICE int getIndex(int row, int column, MatrixOrdering::RowMajor) const {
        return column + row * values.cols();
    }
    PARFAIT_DEVICE int getIndex(int row, int column, MatrixOrdering::ColumnMajor) const {
        return row + column * values.rows();
    }

    PARFAIT_DEVICE constexpr int rows() const { return values.rows(); }
    PARFAIT_DEVICE constexpr int cols() const { return values.cols(); }

    PARFAIT_DEVICE T* data() { return values.data(); }
    PARFAIT_DEVICE const T* data() const { return values.data(); }

    PARFAIT_DEVICE void fill(const T& z) {
        for (int i = 0; i < rows() * cols(); ++i) values.m[i] = z;
    }

    bool isFinite() const {
        for (int i = 0; i < rows() * cols(); ++i) {
            if (not std::isfinite(values.m[i])) return false;
        }
        return true;
    }

    PARFAIT_DEVICE void clear() { fill(0.0); }

    PARFAIT_DEVICE static IdentityMatrix<T, Rows, Columns> Identity() { return {}; }
    PARFAIT_DEVICE static IdentityMatrix<T, Rows, Columns> Identity(int rows) { return {rows}; }
    PARFAIT_DEVICE static IdentityMatrix<T, Rows, Columns> Identity(int rows, int columns) { return {rows, columns}; }

    PARFAIT_DEVICE void transposeInPlace() {
        DenseMatrix<T, Rows, Columns> transpose = this->transpose();
        *this = transpose;
    }

    using MatrixBase<DenseMatrix>::block;
    using SubMatrix = MutableSubMatrix<DenseMatrix>;
    PARFAIT_DEVICE SubMatrix block(int row_start, int col_start, int num_rows, int num_cols) {
        return {row_start, col_start, num_rows, num_cols, *this};
    }

    MatrixStorage<T, Rows, Columns> values;

    void setValues(const std::initializer_list<T>& rhs) {
        int i = 0;
        for (const T& v : rhs) values.m[i++] = v;
    }
    void setValues(const std::initializer_list<std::initializer_list<T>>& rhs) {
        int row = 0;
        for (const auto& row_values : rhs) {
            int col = 0;
            for (const auto& value : row_values) {
                this->operator()(row, col++) = value;
            }
            row++;
        }
    }
    template <typename LHS, typename RHS>
    PARFAIT_DEVICE static void setValues(MatrixBase<LHS>& lhs, const MatrixBase<RHS>& rhs) {
        auto& self = lhs.cast();
        for (int row = 0; row < self.rows(); ++row) {
            for (int col = 0; col < self.cols(); ++col) {
                self(row, col) = rhs.cast()(row, col);
            }
        }
    }
};

template <typename T, int Length>
using Vector = DenseMatrix<T, Length, 1>;

template <typename T>
PARFAIT_DEVICE DenseMatrix<T, 3, 3> inverse(const DenseMatrix<T, 3, 3>& M) {
    T det = M(0, 0) * (M(1, 1) * M(2, 2) - M(2, 1) * M(1, 2)) - M(0, 1) * (M(1, 0) * M(2, 2) - M(1, 2) * M(2, 0)) +
            M(0, 2) * (M(1, 0) * M(2, 1) - M(1, 1) * M(2, 0));

    double invdet = 1 / det;

    DenseMatrix<T, 3, 3> minv(3, 3);  // inverse of matrix m
    minv(0, 0) = (M(1, 1) * M(2, 2) - M(2, 1) * M(1, 2)) * invdet;
    minv(0, 1) = (M(0, 2) * M(2, 1) - M(0, 1) * M(2, 2)) * invdet;
    minv(0, 2) = (M(0, 1) * M(1, 2) - M(0, 2) * M(1, 1)) * invdet;
    minv(1, 0) = (M(1, 2) * M(2, 0) - M(1, 0) * M(2, 2)) * invdet;
    minv(1, 1) = (M(0, 0) * M(2, 2) - M(0, 2) * M(2, 0)) * invdet;
    minv(1, 2) = (M(1, 0) * M(0, 2) - M(0, 0) * M(1, 2)) * invdet;
    minv(2, 0) = (M(1, 0) * M(2, 1) - M(2, 0) * M(1, 1)) * invdet;
    minv(2, 1) = (M(2, 0) * M(0, 1) - M(0, 0) * M(2, 1)) * invdet;
    minv(2, 2) = (M(0, 0) * M(1, 1) - M(1, 0) * M(0, 1)) * invdet;

    return minv;
}

class GaussJordan {
  public:
    template <typename T, int ROWS, int COLS>
    static Vector<T, ROWS> solve(const DenseMatrix<T, ROWS, COLS>& A_in, const Vector<T, ROWS>& b_in) {
        DenseMatrix<T, ROWS, COLS> A = A_in;
        Vector<T, ROWS> b = b_in;
        Vector<int, ROWS> pivot;
        for (int i = 0; i < ROWS; i++) pivot[i] = i;
        gaussianElimination(A, b, pivot);
        backsubstitution(A, b, pivot);

        Vector<T, ROWS> x;
        for (int i = 0; i < ROWS; i++) x[i] = b[pivot[i]];
        return x;
    }

  private:
    template <typename T, int ROWS, int COLS>
    static void backsubstitution(DenseMatrix<T, ROWS, COLS>& A, Vector<T, ROWS>& b, Vector<int, ROWS>& pivot) {
        for (int col = COLS - 1; col >= 0; col--) {
            for (int row = 0; row < col; row++) {
                auto scaling_factor = A(pivot[row], col) / A(pivot[col], col);
                for (int j = 0; j < COLS; j++) A(pivot[row], j) -= scaling_factor * A(pivot[col], j);
                b[pivot[row]] -= scaling_factor * b[pivot[col]];
            }
            b[pivot[col]] /= A(pivot[col], col);
        }
    }

    template <typename T, int ROWS, int COLS>
    static void gaussianElimination(DenseMatrix<T, ROWS, COLS>& A, Vector<T, ROWS>& b, Vector<int, ROWS>& pivot) {
        for (int col = 0; col < COLS - 1; col++) {
            partialPivot(A, b, col, pivot);
            for (int row = col + 1; row < ROWS; row++) {
                auto scaling_factor = A(pivot[row], col) / A(pivot[col], col);
                for (int j = 0; j < COLS; j++) A(pivot[row], j) -= scaling_factor * A(pivot[col], j);
                b[pivot[row]] -= scaling_factor * b[pivot[col]];
            }
        }
    }

    template <typename T, int ROWS, int COLS>
    static void partialPivot(DenseMatrix<T, ROWS, COLS>& A,
                             Vector<T, ROWS>& b,
                             int current_row,
                             Vector<int, ROWS>& pivot) {
        int max_row = current_row;
        int current_col = current_row;
        for (int row = current_row + 1; row < ROWS; row++) {
            int r = pivot[row];
            if (square(A(r, current_col)) > square(A(pivot[max_row], current_col))) max_row = row;
        }
        if (max_row != current_row) std::swap(pivot[current_row], pivot[max_row]);
    }

    template <typename T>
    PARFAIT_DEVICE static T square(const T& t) {
        return t * t;
    }
};

template <typename T, int Rows, int Columns, class Indexing>
struct MatrixTraits<DenseMatrix<T, Rows, Columns, Indexing>> {
    using value_type = T;
};

template <typename T, int Rows, int Columns, class Indexing>
struct MatrixTraits<MutableSubMatrix<DenseMatrix<T, Rows, Columns, Indexing>>> {
    using value_type = T;
};

}
