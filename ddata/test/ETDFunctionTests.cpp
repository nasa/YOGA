#include <ddata/ComplexDifferentiation.h>
#include <ddata/ETD.h>
#include <ddata/DTD.h>
#include <set>
#include <RingAssertions.h>
#include "TestHelpers.h"

using namespace Linearize;

#define TEST_BINARY(a, b, function, expected_value, expected_derivatives)                   \
    {                                                                                       \
        auto result = Linearize::function(a, b);                                            \
        REQUIRE(std::real(expected_value) == Approx(std::real(result.f())));                \
        for (size_t i = 0; i < (expected_derivatives).size(); i++) {                        \
            INFO("derivative: " << i);                                                      \
            REQUIRE(std::real(expected_derivatives[i]) == Approx(std::real(result.dx(i)))); \
        }                                                                                   \
    }

#define TEST_UNARY(a, function, expected_value, expected_derivatives)                       \
    {                                                                                       \
        auto result = Linearize::function(a);                                               \
        REQUIRE(std::real(expected_value) == Approx(std::real(result.f())));                \
        for (size_t i = 0; i < (expected_derivatives).size(); i++) {                        \
            INFO("derivative: " << i);                                                      \
            REQUIRE(std::real(expected_derivatives[i]) == Approx(std::real(result.dx(i)))); \
        }                                                                                   \
    }

template <typename T, class ADClass>
void testETDType() {
    ADClass ddt1(-2.0, std::array<T, 5>{1, 2, 3, 4, 5});
    ADClass ddt2(3.0, std::array<T, 5>{1.1, 2.1, 3.1, 4.1, 5.1});
    T scalar = 0.25;
    SECTION("max") {
        TEST_BINARY(ddt1, ddt2, max, ddt2.f(), extractDerivatives(ddt2));
        std::array<T, 5> expected_derivatives{};
        TEST_BINARY(ddt1, scalar, max, scalar, expected_derivatives);
        TEST_BINARY(scalar, ddt1, max, scalar, expected_derivatives);
        TEST_BINARY(-3.0, ddt1, max, ddt1.f(), extractDerivatives(ddt1));
    }
    SECTION("min") {
        auto expected_derivatives = extractDerivatives(ddt1);
        TEST_BINARY(ddt1, ddt2, min, ddt1.f(), expected_derivatives);
        TEST_BINARY(ddt1, scalar, min, ddt1.f(), expected_derivatives);
        TEST_BINARY(scalar, ddt1, min, ddt1.f(), expected_derivatives);
        std::fill(expected_derivatives.begin(), expected_derivatives.end(), 0.0);
        TEST_BINARY(-5.0, ddt1, min, -5.0, expected_derivatives);
    }
    SECTION("abs") {
        auto expected_derivatives = calcDerivatives(ddt1, std::negate<>());
        TEST_UNARY(ddt1, abs, 2.0, expected_derivatives)
    }
    SECTION("pow") {
        auto expected_derivatives = calcDerivatives(ddt2, ddt1, [=](auto rhs, auto r) {
            return ddt1.f() * std::pow(ddt2.f(), (ddt1.f() - 1.0)) * rhs +
                   std::log(ddt2.f()) * std::pow(ddt2.f(), ddt1.f()) * r;
        });
        TEST_BINARY(ddt2, ddt1, pow, std::pow(ddt2.f(), ddt1.f()), expected_derivatives)

        expected_derivatives =
            calcDerivatives(ddt2, [=](auto v) { return scalar * std::pow(ddt2.f(), scalar - 1.0) * v; });
        TEST_BINARY(ddt2, scalar, pow, std::pow(ddt2.f(), scalar), expected_derivatives)

        expected_derivatives =
            calcDerivatives(ddt2, [=](auto v) { return std::pow(scalar, ddt2.f()) * std::log(scalar) * v; });
        TEST_BINARY(scalar, ddt2, pow, std::pow(scalar, ddt2.f()), expected_derivatives)
    }
    SECTION("exp") {
        auto expected_derivatives = calcDerivatives(ddt1, [=](auto v) { return std::exp(ddt1.f()) * v; });
        TEST_UNARY(ddt1, exp, std::exp(ddt1.f()), expected_derivatives)
    }
    SECTION("log") {
        auto expected_derivatives = calcDerivatives(ddt2, [=](auto v) { return v / ddt2.f(); });
        TEST_UNARY(ddt2, log, std::log(ddt2.f()), expected_derivatives)
    }
    SECTION("sqrt") {
        auto expected_derivatives = calcDerivatives(ddt2, [=](auto v) { return 0.5 / std::sqrt(ddt2.f()) * v; });
        TEST_UNARY(ddt2, sqrt, std::sqrt(ddt2.f()), expected_derivatives)
    }
    SECTION("sin") {
        auto expected_derivatives = calcDerivatives(ddt2, [=](auto v) { return std::cos(ddt2.f()) * v; });
        TEST_UNARY(ddt2, sin, std::sin(ddt2.f()), expected_derivatives)
    }
    SECTION("cos") {
        auto expected_derivatives = calcDerivatives(ddt2, [=](auto v) { return -std::sin(ddt2.f()) * v; });
        TEST_UNARY(ddt2, cos, std::cos(ddt2.f()), expected_derivatives)
    }
    SECTION("asin") {
        ETD<2> ddt(0.5, {1.0, 2.0});
        auto expected_derivatives =
            calcDerivatives(ddt, [=](auto v) { return 1.0 / std::sqrt(1.0 - ddt.f() * ddt.f()) * v; });
        TEST_UNARY(ddt, asin, std::asin(ddt.f()), expected_derivatives)
    }
    SECTION("acos") {
        ETD<2> ddt(0.5, {1.0, 2.0});
        auto expected_derivatives =
            calcDerivatives(ddt, [=](auto v) { return -1.0 / std::sqrt(1.0 - ddt.f() * ddt.f()) * v; });
        TEST_UNARY(ddt, acos, std::acos(ddt.f()), expected_derivatives)
    }
    SECTION("tanh") {
        auto expected_derivatives = calcDerivatives(ddt2, [=](auto v) {
            auto f = std::tanh(ddt2.f());
            return (1.0 - f * f) * v;
        });
        TEST_UNARY(ddt2, tanh, std::tanh(ddt2.f()), expected_derivatives)
    }
    SECTION("real") { REQUIRE(ddt1.f() == Linearize::real(ddt1)); }
}

using complex = std::complex<double>;

TEMPLATE_TEST_CASE("Scalar STL functions", "[scalar]", (float), (double), (complex)) {
    testETDType<TestType, ETD<5, TestType>>();
    testETDType<TestType, DTD<TestType>>();
}

TEST_CASE("Extract the jacobian from ETD ") {
    std::array<double, 5> q1 = {2.0, 3.0, 4.0, 5.0, 6.0};
    auto test_ddt = ETD<5>::Identity(q1);

    auto jacobian = ExtractBlockJacobian(test_ddt);

    std::set<int> diagonal = {0, 6, 12, 18, 24};

    for (int i = 0; i < 25; ++i) {
        if (diagonal.count(i) == 0) {
            REQUIRE(jacobian[i] == 0.0);
        } else {
            REQUIRE(jacobian[i] == 1.0);
        }
    }
}

TEST_CASE("ETD constructor") {
    SECTION("value and initializer list") {
        ETD<5> density(1.225, {1, 0, 0, 0, 0});
        REQUIRE(density.f() == 1.225);
        REQUIRE(density.dx(0) == 1);
        REQUIRE(density.dx(1) == 0);
        REQUIRE(density.dx(2) == 0);
        REQUIRE(density.dx(3) == 0);
        REQUIRE(density.dx(4) == 0);
    }
    SECTION("value and array") {
        double func = 0.5;
        std::array<double, 5> deriv = {1.0, 2.0, 3.0, 4.0, 5.0};

        ETD<5> test_ddt(func, deriv);
        REQUIRE(test_ddt.f() == func);
        REQUIRE(test_ddt.dx(0) == 1.0);
        REQUIRE(test_ddt.dx(1) == 2.0);
        REQUIRE(test_ddt.dx(2) == 3.0);
        REQUIRE(test_ddt.dx(3) == 4.0);
        REQUIRE(test_ddt.dx(4) == 5.0);
    }
    SECTION("ETDExpression") {
        ETD<5> a(1.5, {1, 1, 1, 1, 1});
        ETD<5> b(3.2, {0, 1, 2, 3, 4});
        ETD<5> c = a + b;
        REQUIRE(a.f() + b.f() == c.f());
        for (int i = 0; i < 5; ++i) {
            REQUIRE(a.dx(i) + b.dx(i) == c.dx(i));
        }
    }
    SECTION("Indentity") {
        std::array<double, 5> func = {1.0, 2.0, 3.0, 4.0, 5.0};

        auto test_ddt = ETD<5>::Identity(func);

        REQUIRE(test_ddt[0].f() == func[0]);
        REQUIRE(test_ddt[1].f() == func[1]);
        REQUIRE(test_ddt[2].f() == func[2]);
        REQUIRE(test_ddt[3].f() == func[3]);
        REQUIRE(test_ddt[4].f() == func[4]);

        int n = 5;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i == j) break;
                REQUIRE(test_ddt[i].dx(j) == 0.0);
            }
            REQUIRE(test_ddt[i].dx(i) == 1.0);
        }
    }
}