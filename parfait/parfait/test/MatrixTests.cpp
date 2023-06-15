#include <RingAssertions.h>
#include <parfait/DenseMatrix.h>
#include <parfait/Throw.h>

using namespace Parfait;

TEST_CASE("Dense matrix initialization") {
    using Matrix = DenseMatrix<double, Dynamic, Dynamic>;
    SECTION("construct vector with initializer list") {
        Vector<double, Dynamic> b = {0, 2, 4};
        REQUIRE(0.0 == b[0]);
        REQUIRE(2.0 == b[1]);
        REQUIRE(4.0 == b[2]);
    }
    SECTION("construct matrix with initializer lists") {
        Matrix A = {{0, 1, 2}, {3, 4, 5}};
        REQUIRE(0.0 == A(0, 0));
        REQUIRE(1.0 == A(0, 1));
        REQUIRE(2.0 == A(0, 2));
        REQUIRE(3.0 == A(1, 0));
        REQUIRE(4.0 == A(1, 1));
        REQUIRE(5.0 == A(1, 2));
    }
    SECTION("assign matrix to initializer lists") {
        Matrix A(2, 3);
        A = {{0, 1, 2}, {3, 4, 5}};
        REQUIRE(0.0 == A(0, 0));
        REQUIRE(1.0 == A(0, 1));
        REQUIRE(2.0 == A(0, 2));
        REQUIRE(3.0 == A(1, 0));
        REQUIRE(4.0 == A(1, 1));
        REQUIRE(5.0 == A(1, 2));
    }
}

TEMPLATE_TEST_CASE("matrix expression templates", "[matrix]", MatrixOrdering::RowMajor, MatrixOrdering::ColumnMajor) {
    using Matrix = DenseMatrix<double, 2, 2, TestType>;
    Matrix A = {{1, 2}, {3, 4}};
    SECTION("matrix + matrix") {
        Matrix B = A + A;
        REQUIRE(2.0 == Approx(B(0, 0)));
        REQUIRE(4.0 == Approx(B(0, 1)));
        REQUIRE(6.0 == Approx(B(1, 0)));
        REQUIRE(8.0 == Approx(B(1, 1)));
        B += A;
        REQUIRE(3.0 == Approx(B(0, 0)));
        REQUIRE(6.0 == Approx(B(0, 1)));
        REQUIRE(9.0 == Approx(B(1, 0)));
        REQUIRE(12.0 == Approx(B(1, 1)));
    }
    SECTION("matrix - matrix") {
        Matrix B = A - A;
        REQUIRE(0.0 == Approx(B(0, 0)));
        REQUIRE(0.0 == Approx(B(0, 1)));
        REQUIRE(0.0 == Approx(B(1, 0)));
        REQUIRE(0.0 == Approx(B(1, 1)));
        B -= A;
        REQUIRE(-1.0 == Approx(B(0, 0)));
        REQUIRE(-2.0 == Approx(B(0, 1)));
        REQUIRE(-3.0 == Approx(B(1, 0)));
        REQUIRE(-4.0 == Approx(B(1, 1)));
    }
    SECTION("matrix * matrix") {
        Matrix B = A * A;
        REQUIRE(7.0 == Approx(B(0, 0)));
        REQUIRE(10.0 == Approx(B(0, 1)));
        REQUIRE(15.0 == Approx(B(1, 0)));
        REQUIRE(22.0 == Approx(B(1, 1)));
    }
    SECTION("scalar * matrix") {
        Matrix B = 2.0 * A;
        REQUIRE(2.0 == Approx(B(0, 0)));
        REQUIRE(4.0 == Approx(B(0, 1)));
        REQUIRE(6.0 == Approx(B(1, 0)));
        REQUIRE(8.0 == Approx(B(1, 1)));
        Matrix C = A * 2.0;
        REQUIRE(2.0 == Approx(C(0, 0)));
        REQUIRE(4.0 == Approx(C(0, 1)));
        REQUIRE(6.0 == Approx(C(1, 0)));
        REQUIRE(8.0 == Approx(C(1, 1)));
    }
    SECTION("matrix / scalar") {
        Matrix B = A / 2.0;
        REQUIRE(0.5 == Approx(B(0, 0)));
        REQUIRE(1.0 == Approx(B(0, 1)));
        REQUIRE(1.5 == Approx(B(1, 0)));
        REQUIRE(2.0 == Approx(B(1, 1)));
    }
    SECTION("transpose") {
        auto B = A.transpose();
        REQUIRE(1.0 == Approx(B(0, 0)));
        REQUIRE(3.0 == Approx(B(0, 1)));
        REQUIRE(2.0 == Approx(B(1, 0)));
        REQUIRE(4.0 == Approx(B(1, 1)));
    }
    SECTION("norm") { REQUIRE(std::sqrt(30) == Approx(A.norm())); }
    SECTION("absmax") { REQUIRE(4.0 == Approx(A.absmax())); }
}

TEST_CASE("Special matrix types") {
    SECTION("Zero") {
        ZeroMatrix<double, 3, 3> zero;
        REQUIRE(3 == zero.rows());
        REQUIRE(3 == zero.cols());
        for (int row = 0; row < zero.rows(); ++row) {
            for (int col = 0; col < zero.cols(); ++col) {
                REQUIRE(0.0 == zero(row, col));
            }
        }
    }
    SECTION("Identity") {
        IdentityMatrix<double, 3, 3> I;
        REQUIRE(3 == I.rows());
        REQUIRE(3 == I.cols());
        for (int row = 0; row < I.rows(); ++row) {
            for (int col = 0; col < I.cols(); ++col) {
                auto expected = row == col ? 1.0 : 0.0;
                auto actual = I(row, col);
                REQUIRE(expected == actual);
            }
        }
    }
}

TEST_CASE("Matrix Aliasing") {
    DenseMatrix<double, 2, 2> A = {{1, 2}, {3, 4}};
    A = A * A;
    REQUIRE(7.0 == Approx(A(0, 0)));
    REQUIRE(10.0 == Approx(A(0, 1)));
    REQUIRE(15.0 == Approx(A(1, 0)));
    REQUIRE(22.0 == Approx(A(1, 1)));
}

TEST_CASE("Matrix submatrix block") {
    DenseMatrix<double, 3, 3> A = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    SECTION("Read-only submatrix") {
        auto subblock = A.block(1, 1, 2, 2);
        REQUIRE(2 == subblock.rows());
        REQUIRE(2 == subblock.cols());
        REQUIRE(A(1, 1) == subblock(0, 0));
        REQUIRE(A(1, 2) == subblock(0, 1));
        REQUIRE(A(2, 1) == subblock(1, 0));
        REQUIRE(A(2, 2) == subblock(1, 1));
    }
    SECTION("Read-Write submatrix") {
        auto subblock = A.block(1, 1, 2, 2);
        subblock(0, 0) = 5.5;
        REQUIRE(5.5 == A(1, 1));
        DenseMatrix<double, 2, 2> B = {{0.1, 0.2}, {0.3, 0.4}};
        subblock = B;
        REQUIRE(B(0, 0) == A(1, 1));
        REQUIRE(B(0, 1) == A(1, 2));
        REQUIRE(B(1, 0) == A(2, 1));
        REQUIRE(B(1, 1) == A(2, 2));
    }
}

TEST_CASE("Matrix type conversion") {
    SECTION("Simple cast to scalar") {
        DenseMatrix<int, 1, 1> A = {1};
        int check = A;
        REQUIRE(A[0] == check);
    }
    SECTION("Expression result cast to scalar") {
        DenseMatrix<int, 3, 1> A = {1, 2, 3};
        int result = A.transpose() * A;
        REQUIRE(14 == result);
    }
}