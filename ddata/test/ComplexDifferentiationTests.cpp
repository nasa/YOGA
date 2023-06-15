#include <ddata/ComplexDifferentiation.h>
#include <RingAssertions.h>

// Workaround for Catch2 operator definition headaches
void checkOp(bool r) { REQUIRE(r); }

TEST_CASE("Complex-variable operators") {
    SECTION("comparator operators") {
        std::complex<double> a = {1.0, 0.1};
        std::complex<double> b = {2.0, -0.1};
        checkOp(a < b);
        checkOp(b > a);
        checkOp(a < std::real(b));
        checkOp(b > std::real(a));
        checkOp(std::real(a) < b);
        checkOp(std::real(b) > a);

        checkOp(a <= b);
        checkOp(b >= a);
        checkOp(a <= std::real(b));
        checkOp(b >= std::real(a));
        checkOp(std::real(a) <= b);
        checkOp(std::real(b) >= a);

        checkOp(a <= a);
        checkOp(b >= b);
        checkOp(a <= std::real(a));
        checkOp(b >= std::real(b));
        checkOp(std::real(a) <= a);
        checkOp(std::real(b) <= b);
    }

    SECTION("isnan") {
        std::complex<double> a = {NAN, NAN};
        REQUIRE(Linearize::isnan(a));
        REQUIRE(Linearize::isnan(std::real(a)));
    }

    SECTION("abs - complex") {
        std::complex<double> a = {-1.0, -0.1};
        auto a_abs = Linearize::abs(a);
        REQUIRE(1.0 == a_abs.real());
        REQUIRE(0.1 == a_abs.imag());

        std::complex<double> b = {1.0, 0.1};
        auto b_abs = Linearize::abs(b);
        REQUIRE(1.0 == b_abs.real());
        REQUIRE(0.1 == b_abs.imag());
    }
    SECTION("min/max - complex") {
        std::complex<double> a = {1.0, 0.1};
        std::complex<double> b = {0.1, 0.2};
        REQUIRE(a == Linearize::max(a, b));
        REQUIRE(b == Linearize::min(a, b));
        REQUIRE(a == Linearize::max(a, std::real(b)));
        REQUIRE(b == Linearize::min(std::real(a), b));
        REQUIRE(std::complex<double>(1.0, 0) == Linearize::max(std::real(a), b));
        REQUIRE(std::complex<double>(0.1, 0) == Linearize::min(a, std::real(b)));
    }
    SECTION("real") {
        std::complex<double> a = {1.0, 0.1};
        REQUIRE(1.0 == Linearize::real(a));
    }
    SECTION("pow") {
        std::complex<double> a = {1.0, 0.1};
        auto expected = a * a * a;
        auto actual = Linearize::pow(a, 3);
        REQUIRE(expected.real() == Approx(actual.real()));
        REQUIRE(expected.imag() == Approx(actual.imag()));
    }
}