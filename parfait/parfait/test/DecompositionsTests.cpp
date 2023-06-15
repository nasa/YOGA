#include <RingAssertions.h>
#include <parfait/Decompositions.h>
#include <parfait/DenseMatrix.h>

using namespace Parfait;

template <typename T>
using DynamicMatrix = DenseMatrix<T, Dynamic, Dynamic>;

TEST_CASE("compute upper triangular inverse") {
    double diagonal = 2.0;
    DynamicMatrix<double> A = {{diagonal, 2, 3}, {0, diagonal, 4}, {0, 0, diagonal}};
    auto Ainv = calcUpperTriangularInverse(A);
    auto I = Ainv * A;
    for (int row = 0; row < A.rows(); ++row) {
        for (int col = 0; col < A.cols(); ++col) {
            auto expected = row == col ? 1.0 : 0.0;
            REQUIRE(expected == Approx(I(row, col)));
        }
    }
}

TEST_CASE("Matrix permutation") {
    SECTION("pivoting") {
        DynamicMatrix<double> A = {{.003, 59.14}, {5.291, -6.13}};
        auto pivot = partialPivot(A);
        REQUIRE(1 == pivot[0]);
        REQUIRE(0 == pivot[1]);

        auto A_permuted = A;
        permuteRows(A_permuted, pivot);
        REQUIRE(A_permuted(0, 0) == A(1, 0));
        REQUIRE(A_permuted(0, 1) == A(1, 1));
        REQUIRE(A_permuted(1, 0) == A(0, 0));
        REQUIRE(A_permuted(1, 1) == A(0, 1));
    }
    SECTION("permute rows") {
        DynamicMatrix<double> A{{0, 0}, {1, 1}, {2, 2}, {3, 3}};
        std::vector<int> P = {3, 1, 2, 0};
        permuteRows(A, P);
        for (int row = 0; row < A.rows(); ++row) {
            INFO("Row: " << row);
            CHECK(A(row, 0) == double(P[row]));
            CHECK(A(row, 1) == double(P[row]));
        }
    }
    SECTION("permute columns") {
        DynamicMatrix<double> A = {{0, 1, 2, 3}, {0, 1, 2, 3}};
        std::vector<int> P = {3, 1, 2, 0};
        permuteColumns(A, P);
        for (int col = 0; col < A.cols(); ++col) {
            INFO("Column: " << col);
            CHECK(A(0, col) == double(P[col]));
            CHECK(A(1, col) == double(P[col]));
        }
    }
    SECTION("permute vector") {
        Vector<double, 3> A = {1, 2, 3};
        permuteVector(A, {1, 2, 0});
        CHECK(A[0] == 3.0);
        CHECK(A[1] == 1.0);
        CHECK(A[2] == 2.0);
    }
}

TEST_CASE("QR Factorization") {
    DynamicMatrix<double> A = {{12, -51, 4}, {6, 167, -68}, {-4, 24, -41}};

    auto checkQR =
        [&](const DynamicMatrix<double>& Q, const DynamicMatrix<double>& R, const std::vector<int>& permutation) {
            REQUIRE(3 == Q.rows());
            REQUIRE(3 == Q.cols());
            REQUIRE(3 == R.rows());
            REQUIRE(3 == R.cols());
            {
                DynamicMatrix<double> I = Q * Q.transpose();
                for (int row = 0; row < A.rows(); ++row) {
                    for (int col = 0; col < A.cols(); ++col) {
                        INFO("row: " << row << " col: " << col);
                        auto expected = row == col ? 1.0 : 0.0;
                        REQUIRE(expected == Approx(I(row, col)).margin(1e-8));
                    }
                }
            }
            {
                auto Acheck = Q * R;
                auto AP = A;
                permuteColumns(AP, permutation);
                for (int row = 0; row < A.rows(); ++row) {
                    for (int col = 0; col < A.cols(); ++col) {
                        INFO("row: " << row << " col: " << col);
                        REQUIRE(AP(row, col) == Approx(Acheck(row, col)));
                    }
                }
            }
            {
                DynamicMatrix<double> P = DynamicMatrix<double>::Identity(permutation.size(), permutation.size());
                permuteColumns(P, permutation);

                DynamicMatrix<double> Ainv = P * calcPseudoInverse(Q, R);
                DynamicMatrix<double> I = A * Ainv;
                for (int row = 0; row < A.rows(); ++row) {
                    for (int col = 0; col < A.cols(); ++col) {
                        INFO("row: " << row << " col: " << col);
                        auto expected = row == col ? 1.0 : 0.0;
                        REQUIRE(expected == Approx(I(row, col)).margin(1e-8));
                    }
                }
            }
        };

    SECTION("Using Householder Rotation") {
        auto factorization = householderDecomposition(A);
        checkQR(factorization.Q, factorization.R, factorization.column_pivots);

        {
            auto Ainv = calcPseudoInverse(A);
            REQUIRE((DynamicMatrix<double>::Identity(A.cols(), A.cols()) - A * Ainv).absmax() < 1e-12);
        }
        {
            HouseholderQR<DynamicMatrix<double>> householder_qr(A);
            auto Ainv = householder_qr.calcPseudoInverse();
            REQUIRE((DynamicMatrix<double>::Identity(A.cols(), A.cols()) - A * Ainv).absmax() < 1e-12);
        }
    }
    SECTION("Using Gram-Schmidt") {
        DynamicMatrix<double> Q, R;
        std::tie(Q, R) = gramSchmidtDecomposition(A);
        std::vector<int> P = {0, 1, 2};
        checkQR(Q, R, P);
    }
}

TEST_CASE("use householder QR to determine matrix rank") {
    SECTION("full rank") {
        DynamicMatrix<double> A = {{0, 1, 2}, {2, -3, 4}, {5, 6, -7}};
        HouseholderQR<DynamicMatrix<double>> householder_qr(A);
        REQUIRE(3 == householder_qr.rank());
    }
    SECTION("not full rank") {
        DynamicMatrix<double> A = {{0, 1, 2, -1}, {2, 3, 4, 12}, {0, 1, 2, -1}, {2, 3, 4, -12}, {0, 1, 2, -1}};
        HouseholderQR<DynamicMatrix<double>> householder_qr(A);
        REQUIRE(3 == householder_qr.rank());
    }
}

TEST_CASE("generic solve") {
    SECTION("rows > columns") {
        Vector<double, 3> b = {-1, 7, 2};
        DynamicMatrix<double> A = {{3, -6}, {4, -8}, {0, 1}};
        auto x = solve(A, b);
        REQUIRE(5.0 == Approx(x[0]));
        REQUIRE(2.0 == Approx(x[1]));
    }
    SECTION("rows == columns") {
        Vector<double, 2> b = {5, 2};
        DynamicMatrix<double> A = {{5, -10}, {0, 1}};
        auto x = solve(A, b);
        REQUIRE(5.0 == Approx(x[0]));
        REQUIRE(2.0 == Approx(x[1]));
    }
}

TEST_CASE("Householder column pivoting") {
    DynamicMatrix<double> A = {{27, 21, 9}, {25, 27, 18}, {15, 22, 22}, {2, 7, 29}, {19, 1, 10}, {12, 22, 7}};
    SECTION("Calculate column pivots") {
        std::vector<int> column_pivots = {0, 1, 2};
        householderColumnPivot(column_pivots, A, 0);
        REQUIRE(std::vector<int>{1, 0, 2} == column_pivots);
        householderColumnPivot(column_pivots, A, 1);
        REQUIRE(std::vector<int>{1, 2, 0} == column_pivots);
        householderColumnPivot(column_pivots, A, 2);
        REQUIRE(std::vector<int>{1, 2, 0} == column_pivots);
    }
    SECTION("QR factorization with pivots") {
        auto factorization = householderDecomposition(A);
        REQUIRE(std::vector<int>{1, 2, 0} == factorization.column_pivots);
        auto AP = A;
        permuteColumns(AP, factorization.column_pivots);
        auto diff = ((factorization.Q * factorization.R) - AP).absmax();
        REQUIRE(diff < 1e-12);
    }
}

TEST_CASE("gaussian elimination") {
    SECTION("system that doesn't require pivoting") {
        DenseMatrix<double, 3, 3> m{{-3, -1, 2}, {-2, 1, 2}, {2, 1, -1}};
        Vector<double, 3> b{-11, -3, 8};
        auto x = GaussJordan::solve(m, b);

        REQUIRE(2.0 == Approx(x[0]));
        REQUIRE(3.0 == Approx(x[1]));
        REQUIRE(-1.0 == Approx(x[2]));
    }

    SECTION("enforce pivoting with a system that has an exact answer when reordered, but roundoff in original") {
        DenseMatrix<double, 2, 2> m{{.003, 59.14}, {5.291, -6.13}};
        Vector<double, 2> b{59.17, 46.78};

        auto x = GaussJordan::solve(m, b);
        REQUIRE(10.0 == x[0]);
        REQUIRE(1.0 == x[1]);

        SECTION("m*x should = b") {
            Vector<double, 2> rhs = m * x;
            REQUIRE(rhs[0] == Approx(b[0]));
            REQUIRE(rhs[1] == Approx(b[1]));
        }
    }
}
