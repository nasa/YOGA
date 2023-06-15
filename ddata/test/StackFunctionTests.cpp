#include <RingAssertions.h>
#include <ddata/ETD.h>
#include <ddata/ETDStack.h>

using namespace Linearize;

#define TEST_STACK_UNARY_FUNCTION(value, function)          \
    SECTION(#function) {                                    \
        Var x(value);                                       \
        ETD<1> y(value, 0);                                 \
        ACTIVE_STACK.beginRecording();                      \
        ETD<1> expected = function(y);                      \
        Var actual = function(x);                           \
        actual.setGradient(1.0);                            \
        ACTIVE_STACK.calcAdjoint();                         \
        REQUIRE(expected.f() == Approx(actual.f()));        \
        REQUIRE(expected.dx(0) == Approx(x.getGradient())); \
    }

TEST_CASE("verify StackETD unary functions") {
    TEST_STACK_UNARY_FUNCTION(2.0, sqrt);
    TEST_STACK_UNARY_FUNCTION(2.0, abs);
    TEST_STACK_UNARY_FUNCTION(2.0, exp);
    TEST_STACK_UNARY_FUNCTION(2.0, log);
    TEST_STACK_UNARY_FUNCTION(2.0, tanh);
    TEST_STACK_UNARY_FUNCTION(2.0, sin);
    TEST_STACK_UNARY_FUNCTION(2.0, cos);
    TEST_STACK_UNARY_FUNCTION(0.5, asin);
    TEST_STACK_UNARY_FUNCTION(0.5, acos);
}

TEST_CASE("verify StackETD pow function") {
    ACTIVE_STACK.reset();
    double value1 = 2.0;
    double value2 = 3.0;
    Var x1(value1);
    Var x2(value2);
    ETD<2> y1(value1, 0);
    ETD<2> y2(value2, 1);
    ACTIVE_STACK.beginRecording();
    SECTION("Var-Var") {
        ETD<2> expected = Linearize::pow(y1, y2);
        Var actual = Linearize::pow(x1, x2);
        actual.setGradient(1.0);
        ACTIVE_STACK.calcAdjoint();
        REQUIRE(expected.f() == actual.f());
        REQUIRE(expected.dx(0) == x1.getGradient());
        REQUIRE(expected.dx(1) == x2.getGradient());
    }
    SECTION("Scalar-Var") {
        ETD<2> expected = Linearize::pow(value1, y2);
        Var actual = Linearize::pow(value1, x2);
        actual.setGradient(1.0);
        ACTIVE_STACK.calcAdjoint();
        REQUIRE(expected.f() == actual.f());
        REQUIRE(expected.dx(0) == x1.getGradient());
        REQUIRE(expected.dx(1) == x2.getGradient());
    }
    SECTION("Var-Scalar") {
        ETD<2> expected = Linearize::pow(y1, value2);
        Var actual = Linearize::pow(x1, value2);
        actual.setGradient(1.0);
        ACTIVE_STACK.calcAdjoint();
        REQUIRE(expected.f() == actual.f());
        REQUIRE(expected.dx(0) == x1.getGradient());
        REQUIRE(expected.dx(1) == x2.getGradient());
    }
    SECTION("Var-Int") {
        ETD<2> expected = Linearize::pow(y1, static_cast<int>(value2));
        Var actual = Linearize::pow(x1, static_cast<int>(value2));
        actual.setGradient(1.0);
        ACTIVE_STACK.calcAdjoint();
        REQUIRE(expected.f() == actual.f());
        REQUIRE(expected.dx(0) == x1.getGradient());
        REQUIRE(expected.dx(1) == x2.getGradient());
    }
}

TEST_CASE("verify StackETD min/max") {
    ACTIVE_STACK.reset();
    Var x = 1.0;
    Var y = 2.0;
    ACTIVE_STACK.beginRecording();
    SECTION("max") {
        Var result = std::max(x, y);
        result.setGradient(1.0);
        ACTIVE_STACK.calcAdjoint();
        REQUIRE(y.f() == result.f());
        REQUIRE(0 == x.getGradient());
        REQUIRE(1.0 == y.getGradient());
    }
    SECTION("min") {
        Var result = std::min(x, y);
        result.setGradient(1.0);
        ACTIVE_STACK.calcAdjoint();
        REQUIRE(x.f() == result.f());
        REQUIRE(1.0 == x.getGradient());
        REQUIRE(0.0 == y.getGradient());
    }
}

TEST_CASE("verify StackETD real") {
    Var x(2.0);
    REQUIRE(2.0 == Linearize::real(x));
}