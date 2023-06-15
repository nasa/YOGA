#include <RingAssertions.h>
#include <ddata/ComplexDifferentiation.h>
#include <ddata/ETD.h>
#include <ddata/DTD.h>
#include "TestHelpers.h"

using namespace Linearize;

#define TEST_OPERATOR(a, b, op, expected_value, expected_derivatives)                       \
    {                                                                                       \
        ADClass result = a op b;                                                            \
        REQUIRE(std::real(expected_value) == Approx(std::real(result.f())));                \
        for (size_t i = 0; i < expected_derivatives.size(); i++) {                          \
            INFO("derivative: " << i);                                                      \
            REQUIRE(std::real(expected_derivatives[i]) == Approx(std::real(result.dx(i)))); \
        }                                                                                   \
    }

#define TEST_UNARY_OPERATOR(a, b, op, expected_value, expected_derivatives)                 \
    {                                                                                       \
        auto result = a;                                                                    \
        result op b;                                                                        \
        REQUIRE(std::real(expected_value) == Approx(std::real(result.f())));                \
        for (size_t i = 0; i < expected_derivatives.size(); i++) {                          \
            INFO("derivative: " << i);                                                      \
            REQUIRE(std::real(expected_derivatives[i]) == Approx(std::real(result.dx(i)))); \
        }                                                                                   \
    }

template <typename T, class ADClass>
void testOperators() {
    ADClass ddt1(2.0, std::array<T, 5>{1, 2, 3, 4, 5});
    ADClass ddt2(3.0, std::array<T, 5>{1.1, 2.1, 3.1, 4.1, 5.1});
    T scalar = 0.25;
    SECTION("operator +") {
        std::vector<T> expected_derivatives = {2.1, 4.1, 6.1, 8.1, 10.1};
        TEST_OPERATOR(ddt1, ddt2, +, 5.0, expected_derivatives)
        TEST_UNARY_OPERATOR(ddt1, ddt2, +=, 5.0, expected_derivatives)

        expected_derivatives = extractDerivatives(ddt1);
        TEST_OPERATOR(ddt1, scalar, +, 2.25, expected_derivatives)
        TEST_OPERATOR(scalar, ddt1, +, 2.25, expected_derivatives)
    }
    SECTION("operator -") {
        std::vector<T> expected_derivatives = {-0.1, -0.1, -0.1, -0.1, -0.1};
        TEST_OPERATOR(ddt1, ddt2, -, -1.0, expected_derivatives)
        TEST_UNARY_OPERATOR(ddt1, ddt2, -=, -1.0, expected_derivatives)

        expected_derivatives = extractDerivatives(ddt1);
        TEST_OPERATOR(ddt1, scalar, -, 1.75, expected_derivatives)

        expected_derivatives = calcDerivatives(ddt1, std::negate<>());
        TEST_OPERATOR(scalar, ddt1, -, -1.75, expected_derivatives)
    }
    SECTION("operator *") {
        std::vector<T> expected_derivatives(5);
        for (int i = 0; i < 5; ++i) expected_derivatives[i] = ddt1.f() * ddt2.dx(i) + ddt2.f() * ddt1.dx(i);
        TEST_OPERATOR(ddt1, ddt2, *, 6.0, expected_derivatives)
        TEST_UNARY_OPERATOR(ddt1, ddt2, *=, 6.0, expected_derivatives)

        expected_derivatives = calcDerivatives(ddt1, [=](auto v) { return v * scalar; });
        TEST_OPERATOR(ddt1, scalar, *, 0.5, expected_derivatives)
        TEST_OPERATOR(scalar, ddt1, *, 0.5, expected_derivatives)
    }
    SECTION("operator /") {
        std::vector<T> expected_derivatives(5);
        for (int i = 0; i < 5; ++i)
            expected_derivatives[i] = (ddt2.f() * ddt1.dx(i) - ddt1.f() * ddt2.dx(i)) / (std::pow(ddt2.f(), 2));
        TEST_OPERATOR(ddt1, ddt2, /, 2.0 / 3.0, expected_derivatives)
        TEST_UNARY_OPERATOR(ddt1, ddt2, /=, 2.0 / 3.0, expected_derivatives)

        expected_derivatives = calcDerivatives(ddt1, [=](auto v) { return v / scalar; });
        TEST_OPERATOR(ddt1, scalar, /, 8.0, expected_derivatives)
        expected_derivatives = calcDerivatives(ddt1, [=](auto v) { return -scalar * v / (ddt1.f() * ddt1.f()); });
        TEST_OPERATOR(scalar, ddt1, /, 0.125, expected_derivatives)
    }
    SECTION("comparator >") {
        REQUIRE(ddt2 > ddt1);
        REQUIRE(ddt2 >= ddt1);
        REQUIRE(ddt1 >= ddt1);

        REQUIRE(ddt2 > ddt1.f());
        REQUIRE(ddt2 >= ddt1.f());
        REQUIRE(ddt2 >= ddt2.f());

        REQUIRE(ddt2.f() > ddt1);
        REQUIRE(ddt2.f() >= ddt1);
        REQUIRE(ddt2.f() >= ddt2);
    }
    SECTION("comparator <") {
        REQUIRE(ddt1 < ddt2);
        REQUIRE(ddt1 <= ddt2);
        REQUIRE(ddt1 <= ddt1);

        REQUIRE(ddt1 < ddt2.f());
        REQUIRE(ddt1 <= ddt2.f());
        REQUIRE(ddt1 <= ddt1.f());

        REQUIRE(ddt1.f() < ddt2);
        REQUIRE(ddt1.f() <= ddt2);
        REQUIRE(ddt1.f() <= ddt1);
    }
    SECTION("comparator ==") {
        REQUIRE(ddt1 == ddt1);
        REQUIRE(ddt1 == ddt1.f());
        REQUIRE(ddt1.f() == ddt1);
    }
    SECTION("comparator !=") {
        REQUIRE(ddt2 != ddt1);
        REQUIRE(ddt2 != ddt1.f());
        REQUIRE(ddt2.f() != ddt1);
    }
}

using complex = std::complex<double>;

TEMPLATE_TEST_CASE("Scalar class operators", "[scalar]", (float), (double), (complex)) {
    SECTION("ETD") { testOperators<TestType, ETD<5, TestType>>(); }
    SECTION("DTD") { testOperators<TestType, DTD<TestType>>(); }
}
