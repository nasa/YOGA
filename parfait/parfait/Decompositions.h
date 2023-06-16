#pragma once
// #include "DynamicMatrix.h"
#include "DenseMatrix.h"
#include "Throw.h"
#include <tuple>

namespace Parfait {

template <typename Matrix>
void calcRowPivot(std::vector<int>& pivots, const Matrix& matrix, int current_row) {
    PARFAIT_ASSERT(matrix.rows() == matrix.cols(), "Pivoting requires a square matrix");
    int max_row = current_row;
    int current_col = current_row;
    for (int row = current_row + 1; row < matrix.rows(); row++) {
        if (matrix(row, current_col) > matrix(max_row, current_col)) max_row = row;
    }
    if (max_row != current_row) std::swap(pivots[current_row], pivots[max_row]);
}

template <typename Matrix>
std::vector<int> partialPivot(const Matrix& matrix) {
    PARFAIT_ASSERT(matrix.rows() == matrix.cols(), "Pivoting requires a square matrix");
    std::vector<int> pivots(matrix.rows());
    for (int row = 0; row < matrix.rows(); ++row) pivots[row] = row;
    for (int row = 0; row < matrix.rows(); ++row) calcRowPivot(pivots, matrix, row);
    return pivots;
}

template <typename Matrix>
void permuteRows(Matrix& matrix, const std::vector<int>& permutation) {
    PARFAIT_ASSERT(matrix.rows() == (int)permutation.size(), "Permutation should include all matrix rows");
    Matrix original = matrix;
    for (int row = 0; row < matrix.rows(); ++row) {
        for (int col = 0; col < matrix.cols(); ++col) {
            matrix(row, col) = original(permutation[row], col);
        }
    }
}
template <typename Matrix>
void permuteColumns(Matrix& matrix, const std::vector<int>& permutation) {
    PARFAIT_ASSERT(matrix.cols() == (int)permutation.size(), "Permutation should include all matrix columns");
    Matrix original = matrix;
    for (int col = 0; col < matrix.cols(); ++col) {
        for (int row = 0; row < matrix.rows(); ++row) {
            matrix(row, col) = original(row, permutation[col]);
        }
    }
}

template <typename Vector>
void permuteVector(Vector& vector, const std::vector<int>& permutation) {
    PARFAIT_ASSERT(vector.rows() == 1 or vector.cols() == 1, "Must be a vector");
    Vector v = vector;
    for (int i = 0; i < vector.rows() * vector.cols(); ++i) {
        vector[permutation[i]] = v[i];
    }
}

template <typename Matrix, typename Vector>
auto backsolve(const Matrix& R, const Vector& b) {
    Matrix x(R.cols(), 1);
    for (int row = 0; row < x.rows(); ++row) x[row] = b[row];
    for (int row = x.rows() - 1; row >= 0; --row) {
        for (int column = row + 1; column < R.cols(); ++column) {
            x[row] -= R(row, column) * x[column];
        }
        x[row] /= R(row, row);
    }
    return x;
}
template <typename Matrix, typename Vector>
auto forwardsolve(const Matrix& R, const Vector& b) {
    Matrix x(R.columns, 1);
    for (int row = 0; row < R.rows(); ++row) {
        x[row] = b[row];
        for (int column = 0; column < row; ++column) {
            x[row] -= R(row, column) * x[column];
        }
        x[row] /= R(row, row);
    }
    return x;
}

template <typename Matrix>
void factorLU(Matrix& A, int num_eqns) {
    for (int k = 1; k < num_eqns; ++k) {
        for (int i = k; i < num_eqns; ++i) {
            A(i, k - 1) /= A(k - 1, k - 1);
            for (int j = k; j < num_eqns; ++j) {
                A(i, j) -= A(i, k - 1) * A(k - 1, j);
            }
        }
    }
}

template <typename Matrix, typename Vector>
Vector solveLUPreviouslyFactored(Matrix A, Vector b, int num_eqns) {
    for (int k = 1; k < num_eqns; ++k) {
        for (int i = k; i < num_eqns; ++i) {
            b[i] -= A(i, k - 1) * b[k - 1];
        }
    }
    for (int i = num_eqns - 1; i >= 0; --i) {
        if (i < num_eqns - 1) {
            for (int j = i + 1; j < num_eqns; ++j) {
                b[i] -= A(i, j) * b[j];
            }
        }
        b[i] /= A(i, i);
    }
    return b;
}

template <typename Matrix, typename Vector>
Vector solveLU(Matrix A, Vector b, int num_eqns) {
    factorLU(A, num_eqns);
    return solveLUPreviouslyFactored(A, b, num_eqns);
}

template <typename Matrix>
void householderColumnPivot(std::vector<int>& column_pivots, const Matrix& A, int current_column) {
    using T = decltype(A(0, 0));
    using value_type = typename std::remove_const<typename std::remove_reference<T>::type>::type;
    auto calcColumnNorm = [&](int column) {
        value_type norm = 0.0;
        for (int row = current_column; row < A.rows(); ++row) {
            norm += A(row, column) * A(row, column);
        }
        return norm;
    };

    auto max_column_norm = calcColumnNorm(column_pivots[current_column]);
    int max_column = current_column;
    for (int column = current_column + 1; column < A.cols(); ++column) {
        auto norm = calcColumnNorm(column_pivots[column]);
        if (norm > max_column_norm) {
            max_column_norm = norm;
            max_column = column;
        }
    }
    if (max_column != current_column) std::swap(column_pivots[current_column], column_pivots[max_column]);
}

template <typename Matrix>
Matrix householderReflection(const Matrix& R, int row, int column) {
    int m = R.rows();
    Matrix v(m - row, 1);
    for (int j = 0; j < v.rows(); ++j) v[j] = R(j + row, column);
    auto v_mag = v.norm();
    using T = decltype(v_mag);
    if (v_mag >= 1e-10) {
        v[0] += v[0] >= 0.0 ? v_mag : T(-v_mag);
        v /= v.norm();
    } else {
        v[0] = 1.0;
    }
    return v;
}

template <typename Matrix>
struct QRFactorization {
    Matrix Q;
    Matrix R;
    std::vector<int> column_pivots;
};

template <typename Matrix>
QRFactorization<Matrix> householderDecomposition(const Matrix& A) {
    int m = A.rows();
    int n = A.cols();
    PARFAIT_ASSERT(m >= n,
                   "Cannot perform householderDecomposition on matrix with less rows than columns.  Size: " +
                       std::to_string(m) + "x" + std::to_string(n));
    Matrix Q = Matrix::Identity(m, m);
    Matrix R = A;

    Matrix tmp(m, 1);
    Matrix v(m, 1);

    std::vector<int> column_pivots(n);
    for (int i = 0; i < n; ++i) column_pivots[i] = i;

    for (int i = 0; i < n; ++i) {
        householderColumnPivot(column_pivots, R, i);
        v.block(i, 0, m - i, 1) = householderReflection(R, i, column_pivots[i]);
        // tmp = v^t * R
        tmp.fill(0.0);
        for (int k = i; k < m; ++k) {
            for (int j = 0; j < n; ++j) {
                tmp[j] += v[k] * R(k, j);
            }
        }

        // R = R - 2 * v * (v^t * R)
        for (int row = i; row < m; ++row) {
            for (int col = 0; col < n; ++col) {
                R(row, col) -= 2.0 * v[row] * tmp[col];
            }
        }

        // tmp = Q * v
        for (int j = 0; j < m; ++j) {
            tmp[j] = 0.0;
            for (int k = i; k < m; ++k) {
                tmp[j] += Q(j, k) * v[k];
            }
        }

        // Q = Q - 2 * (Q * v) * v^t
        for (int row = 0; row < m; ++row) {
            for (int col = i; col < m; ++col) {
                Q(row, col) -= 2.0 * tmp[row] * v[col];
            }
        }
    }
    permuteColumns(R, column_pivots);
    return {Q, R, column_pivots};
}

template <class Matrix>
std::tuple<Matrix, Matrix> gramSchmidtDecomposition(const Matrix& A) {
    int m = A.rows();
    int n = A.cols();
    Matrix Q = A;
    Matrix R(n, n);
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < m; i++) R(k, k) += std::pow(Q(i, k), 2);
        R(k, k) = std::sqrt(R(k, k));
        for (int i = 0; i < m; i++) {
            Q(i, k) /= R(k, k);
        }
        for (int j = k + 1; j < n; j++) {
            for (int i = 0; i < m; i++) R(k, j) += A(i, j) * Q(i, k);
            for (int i = 0; i < m; i++) Q(i, j) -= R(k, j) * Q(i, k);
        }
    }
    return {Q, R};
}

template <class Matrix>
Matrix calcUpperTriangularInverse(const Matrix& R) {
    auto m = R.rows();
    auto n = R.cols();
    Matrix Rinv(n, m);
    for (int i = 0; i < std::min(m, n); ++i) Rinv(i, i) = 1.0;
    for (int row = n - 1; row >= 0; row--) {
        Rinv(row, row) /= R(row, row);
        for (int col = row + 1; col < n; ++col) {
            for (int c = col; c < n; ++c) {
                Rinv(row, c) -= R(row, col) / R(row, row) * Rinv(col, c);
            }
        }
    }
    return Rinv;
}

template <class Matrix>
Matrix calcPseudoInverse(const Matrix& Q, const Matrix& R) {
    return calcUpperTriangularInverse(R) * Q.transpose();
}

template <class Matrix>
Matrix calcPseudoInverse(const Matrix& A) {
    auto factorization = householderDecomposition(A);
    Matrix Ainv = calcUpperTriangularInverse(factorization.R) * factorization.Q.transpose();
    Matrix P = Matrix::Identity(A.cols(), A.cols());
    permuteColumns(P, factorization.column_pivots);
    return P * Ainv;
}

template <typename Matrix>
class HouseholderQR {
  public:
    HouseholderQR(const Matrix& A) : A(A), column_pivots(A.cols()), R(A) { calcR(); }
    Matrix calcPseudoInverse() const {
        Matrix R_inv = calcRInverse();
        Matrix Q = A * R_inv;
        return R_inv * Q.transpose();
    }
    int rank(double threshold = 1e-14) const {
        for (int rank = A.cols(); rank > 0; --rank) {
            int row = rank - 1;
            int col = column_pivots[row];
            if (std::abs(R(row, col)) > threshold) return rank;
        }
        return 0;
    }
    const Matrix& getR() const { return R; }
    Matrix calcQ() const { return A * calcRInverse(); }
    Matrix calcRInverse() const {
        int n = A.cols();
        Matrix P = Matrix::Identity(A.cols(), A.cols());
        permuteColumns(P, column_pivots);

        Matrix R_nxn = R.block(0, 0, n, n);
        permuteColumns(R_nxn, column_pivots);
        return P * calcUpperTriangularInverse(R_nxn);
    }
    const std::vector<int>& getColumnPivots() const { return column_pivots; }

  private:
    const Matrix& A;
    std::vector<int> column_pivots;
    Matrix R;

    void calcR() {
        int m = A.rows();
        int n = A.cols();

        Matrix tmp(m, 1);
        Matrix v(m, 1);
        for (int i = 0; i < n; ++i) column_pivots[i] = i;
        for (int i = 0; i < n; ++i) {
            householderColumnPivot(column_pivots, R, i);
            v.block(i, 0, m - i, 1) = householderReflection(R, i, column_pivots[i]);
            // tmp = v^t * R
            tmp.fill(0.0);
            for (int k = i; k < m; ++k) {
                for (int j = 0; j < n; ++j) {
                    tmp[j] += v[k] * R(k, j);
                }
            }

            // R = R - 2 * v * (v^t * R)
            for (int row = i; row < m; ++row) {
                for (int col = 0; col < n; ++col) {
                    R(row, col) -= 2.0 * v[row] * tmp[col];
                }
            }
        }
    }
};

template <typename Matrix, typename Vector>
Matrix solve(const Matrix& A, const Vector& b) {
    PARFAIT_ASSERT(A.rows() >= A.cols(), "solve requires matrix with rows >= columns");
    if (A.rows() == A.cols()) return solveLU(A, b, A.rows());

    // Overdetermined problem
    auto factorization = householderDecomposition(A);
    Vector Qb = factorization.Q.transpose() * b;
    auto x = backsolve(factorization.R, Qb);
    permuteVector(x, factorization.column_pivots);
    return x;
}
}
