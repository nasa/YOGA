#include <RingAssertions.h>
#include <ddata/ETDStack.h>
#include <ddata/ETD.h>

using namespace Linearize;

#define TEST_OPERATOR(op)                          \
    ETD<2> check = ETD<2>{a, 0} op ETD<2>{b, 1};   \
    Var result = x op y;                           \
    result.setGradient(1.0);                       \
    ACTIVE_STACK.calcAdjoint();                    \
    CHECK(a op b == Approx(result.f()));           \
    CHECK(check.dx(0) == Approx(x.getGradient())); \
    CHECK(check.dx(1) == Approx(y.getGradient()));

#define TEST_LEFT_SCALAR_OPERATOR(op)              \
    ETD<2> check = a op ETD<2>{b, 1};              \
    Var result = a op y;                           \
    result.setGradient(1.0);                       \
    ACTIVE_STACK.calcAdjoint();                    \
    CHECK(a op b == Approx(result.f()));           \
    CHECK(check.dx(0) == Approx(x.getGradient())); \
    CHECK(check.dx(1) == Approx(y.getGradient()));

#define TEST_RIGHT_SCALAR_OPERATOR(op)             \
    ETD<2> check = ETD<2>{a, 0} op b;              \
    Var result = x op b;                           \
    result.setGradient(1.0);                       \
    ACTIVE_STACK.calcAdjoint();                    \
    CHECK(a op b == Approx(result.f()));           \
    CHECK(check.dx(0) == Approx(x.getGradient())); \
    CHECK(check.dx(1) == Approx(y.getGradient()));

TEST_CASE("Verify StackETD Var-Var binary operators") {
    double a = 2.0;
    double b = 3.0;
    Var x(a);
    Var y(b);
    ACTIVE_STACK.beginRecording();
    SECTION("Var-Var") {
        SECTION("operator +") { TEST_OPERATOR(+); }
        SECTION("operator -") { TEST_OPERATOR(-); }
        SECTION("operator *") { TEST_OPERATOR(*); }
        SECTION("operator /") { TEST_OPERATOR(/); }
    }
    SECTION("Scalar-Var") {
        SECTION("operator +") { TEST_LEFT_SCALAR_OPERATOR(+); }
        SECTION("operator -") { TEST_LEFT_SCALAR_OPERATOR(-); }
        SECTION("operator *") { TEST_LEFT_SCALAR_OPERATOR(*); }
        SECTION("operator /") { TEST_LEFT_SCALAR_OPERATOR(/); }
    }
    SECTION("Var-Scalar") {
        SECTION("operator +") { TEST_RIGHT_SCALAR_OPERATOR(+); }
        SECTION("operator -") { TEST_RIGHT_SCALAR_OPERATOR(-); }
        SECTION("operator *") { TEST_RIGHT_SCALAR_OPERATOR(*); }
        SECTION("operator /") { TEST_RIGHT_SCALAR_OPERATOR(/); }
    }
}

#define TEST_UNARY_OPERATOR(op, value1, value2)    \
    SECTION(#op) {                                 \
        ETD<2> b(value2, 1);                       \
        b op ETD<2>{value1, 0};                    \
        Var y(value2);                             \
        Var x(value1);                             \
        ACTIVE_STACK.beginRecording();             \
        y op x;                                    \
        y.setGradient(1.0);                        \
        ACTIVE_STACK.calcAdjoint();                \
        INFO(ACTIVE_STACK);                        \
        CHECK(b.f() == Approx(y.f()));             \
        CHECK(b.dx(0) == Approx(x.getGradient())); \
        CHECK(b.dx(1) == Approx(y.getGradient())); \
    }

#define TEST_UNARY_SCALAR_OPERATOR(op, value1, value2) \
    SECTION(#op) {                                     \
        Var y(value2);                                 \
        ACTIVE_STACK.beginRecording();                 \
        y op value1;                                   \
        y.setGradient(1.0);                            \
        ACTIVE_STACK.calcAdjoint();                    \
        INFO(ACTIVE_STACK);                            \
        auto result = value2;                          \
        result op value1;                              \
        CHECK(result == Approx(y.f()));                \
        CHECK(1.0 == y.getGradient());                 \
    }

TEST_CASE("Verify StackETD unary operators") {
    SECTION("Var") {
        TEST_UNARY_OPERATOR(+=, 2.0, 3.0);
        TEST_UNARY_OPERATOR(-=, 2.0, 3.0);
        TEST_UNARY_OPERATOR(*=, 2.0, 3.0);
        TEST_UNARY_OPERATOR(/=, 2.0, 3.0);
    }
    SECTION("Scalar") {
        TEST_UNARY_SCALAR_OPERATOR(+=, 2.0, 3.0);
        TEST_UNARY_SCALAR_OPERATOR(-=, 2.0, 3.0);
        TEST_UNARY_SCALAR_OPERATOR(*=, 2.0, 3.0);
        TEST_UNARY_SCALAR_OPERATOR(/=, 2.0, 3.0);
    }
}

TEST_CASE("Verify StackETD negate operator") {
    ACTIVE_STACK.reset();
    Var x(2.0);
    ACTIVE_STACK.beginRecording();
    Var y = -x;
    y.setGradient(1.0);
    ACTIVE_STACK.calcAdjoint();
    REQUIRE(-2.0 == y.f());
    REQUIRE(-1.0 == x.getGradient());
}

TEST_CASE("Verify StackETD comparators") {
    ACTIVE_STACK.reset();
    Var a(2.0);
    Var b(3.0);
    SECTION("comparator >") {
        REQUIRE(b > a);
        REQUIRE(b >= a);
        REQUIRE(a >= a);

        REQUIRE(b > a.f());
        REQUIRE(b >= a.f());
        REQUIRE(b >= b.f());

        REQUIRE(b.f() > a);
        REQUIRE(b.f() >= a);
        REQUIRE(b.f() >= b);
    }
    SECTION("comparator <") {
        REQUIRE(a < b);
        REQUIRE(a <= b);
        REQUIRE(a <= a);

        REQUIRE(a < b.f());
        REQUIRE(a <= b.f());
        REQUIRE(a <= a.f());

        REQUIRE(a.f() < b);
        REQUIRE(a.f() <= b);
        REQUIRE(a.f() <= a);
    }
    SECTION("comparator ==") {
        REQUIRE(a == a);
        REQUIRE(a == a.f());
        REQUIRE(a.f() == a);
    }
    SECTION("comparator !=") {
        REQUIRE(b != a);
        REQUIRE(b != a.f());
        REQUIRE(b.f() != a);
    }
}