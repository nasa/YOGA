#include <RingAssertions.h>
#include <ddata/ETDStack.h>
#include <ddata/ETD.h>

using namespace Linearize;

// Example function from Adept publication (by R. J. Hogan)
template <typename T>
T func(const T& x1, const T& x2) {
    T y = 4.0;
    T s = 2.0 * x1 + 3.0 * x2 * x2;
    y += sin(s);
    return y;
}

TEST_CASE("Stack-based ETD") {
    ACTIVE_STACK.reset();
    Var x1(1.0);
    Var x2(2.0);
    ACTIVE_STACK.beginRecording();
    SECTION("constructor sets gradient index") {
        REQUIRE(1.0 == x1.value);
        REQUIRE(2.0 == x2.value);
        REQUIRE(0 == x1.gradient_index);
        REQUIRE(1 == x2.gradient_index);
        REQUIRE(2 == ACTIVE_STACK.num_gradients);
    }
    SECTION("Can set gradients") {
        x2.setGradient(1.0);
        REQUIRE(0.0 == ACTIVE_STACK.gradients[0]);
        REQUIRE(1.0 == ACTIVE_STACK.gradients[1]);
    }
    SECTION("Can get gradients") {
        ACTIVE_STACK.gradients.resize(ACTIVE_STACK.num_gradients);
        ACTIVE_STACK.gradients.back() = 2.5;
        REQUIRE(2.5 == x2.getGradient());
    }
    SECTION("Can compute gradient from simple assignment") {
        Var Y = x2;
        Y.setGradient(1.0);
        ACTIVE_STACK.calcAdjoint();
        REQUIRE(1.0 == x2.getGradient());
    }
    SECTION("Can compute gradient from a multiply") {
        Var Y = x1 * x2;
        Y.setGradient(1.0);
        INFO(ACTIVE_STACK);
        ACTIVE_STACK.calcAdjoint();
        CHECK(2.0 == x1.getGradient());
        CHECK(1.0 == x2.getGradient());
    }
    SECTION("Can compute gradient from function") {
        Var Y = func(x1, x2);
        Y.setGradient(1.0);
        INFO(ACTIVE_STACK);
        ACTIVE_STACK.calcAdjoint();
        ETD<2> a(x1.f(), 0);
        ETD<2> b(x2.f(), 1);
        auto expected = func(a, b);
        CHECK(expected.dx(0) == x1.getGradient());
        CHECK(expected.dx(1) == x2.getGradient());
    }
}
