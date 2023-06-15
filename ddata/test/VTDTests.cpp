#include <RingAssertions.h>
#include <ddata/ETD.h>
#include <ddata/ETDv.h>
#include <ddata/VTD.h>

using namespace Linearize;

#define BINARY_OP(op)                                               \
    SECTION(std::string("operator ") + #op) {                       \
        SECTION("VTD-VTD") {                                        \
            AD result = ddt1 op ddt2;                               \
            ETD<2> expected = e1 op e2;                             \
            REQUIRE(expected.f() == Approx(result.f()));            \
            REQUIRE(expected.dx(0) == Approx(result.dx_scalar(0))); \
            REQUIRE(expected.dx(1) == Approx(result.dx_scalar(1))); \
        }                                                           \
        SECTION("scalar-VTD") {                                     \
            AD result = ddt1.f() op ddt2;                           \
            ETD<2> expected = e1.f() op e2;                         \
            REQUIRE(expected.f() == Approx(result.f()));            \
            REQUIRE(expected.dx(0) == Approx(result.dx_scalar(0))); \
            REQUIRE(expected.dx(1) == Approx(result.dx_scalar(1))); \
        }                                                           \
        SECTION("VTD-scalar") {                                     \
            AD result = ddt1 op ddt2.f();                           \
            ETD<2> expected = e1 op e2.f();                         \
            REQUIRE(expected.f() == Approx(result.f()));            \
            REQUIRE(expected.dx(0) == Approx(result.dx_scalar(0))); \
            REQUIRE(expected.dx(1) == Approx(result.dx_scalar(1))); \
        }                                                           \
    }
#define UNARY_OP(op)                                                \
    SECTION(std::string("operator ") + #op) {                       \
        SECTION("VTD-VTD") {                                        \
            AD result = ddt1;                                       \
            result op ddt2;                                         \
            ETD<2> expected = e1;                                   \
            expected op e2;                                         \
            REQUIRE(expected.f() == Approx(result.f()));            \
            REQUIRE(expected.dx(0) == Approx(result.dx_scalar(0))); \
            REQUIRE(expected.dx(1) == Approx(result.dx_scalar(1))); \
        }                                                           \
        SECTION("VTD-scalar") {                                     \
            AD result = ddt1;                                       \
            result op ddt2.f();                                     \
            ETD<2> expected = e1;                                   \
            expected op e2.f();                                     \
            REQUIRE(expected.f() == Approx(result.f()));            \
            REQUIRE(expected.dx(0) == Approx(result.dx_scalar(0))); \
            REQUIRE(expected.dx(1) == Approx(result.dx_scalar(1))); \
        }                                                           \
    }
#define UNARY_FUNCTION(function)                                \
    SECTION(std::string("function ") + #function) {             \
        AD result = function(ddt1);                             \
        ETD<2> expected = function(e1);                         \
        REQUIRE(expected.f() == Approx(result.f()));            \
        REQUIRE(expected.dx(0) == Approx(result.dx_scalar(0))); \
        REQUIRE(expected.dx(1) == Approx(result.dx_scalar(1))); \
    }

template <typename T, class AD>
void testClass(const AD& ddt1, const AD& ddt2) {
    ETD<2, T> e1(ddt1.f(), 0);
    ETD<2, T> e2(ddt2.f(), 1);
    BINARY_OP(+)
    BINARY_OP(-)
    BINARY_OP(*)
    BINARY_OP(/)
    UNARY_OP(+=)
    UNARY_OP(-=)
    UNARY_OP(*=)
    UNARY_OP(/=)
    UNARY_FUNCTION(abs)
    UNARY_FUNCTION(exp)
    UNARY_FUNCTION(log)
    UNARY_FUNCTION(sqrt)
    UNARY_FUNCTION(sin)
    UNARY_FUNCTION(cos)
    UNARY_FUNCTION(tanh)
    // FIXME: These produce NaNs
    //    UNARY_FUNCTION(asin)
    //    UNARY_FUNCTION(acos)
}

TEMPLATE_TEST_CASE("VTD Operations/Functions", "[vectorized]", (float), (double)) {
    Linearize::VTD<TestType> ddt1(2, 1.0, 0);
    Linearize::VTD<TestType> ddt2(2, 2.0, 1);
    testClass<TestType>(ddt1, ddt2);
}
TEMPLATE_TEST_CASE("ETDv Operations/Functions", "[vectorized]", (float), (double)) {
    Linearize::ETDv<2, TestType> ddt1(1.0, 0);
    Linearize::ETDv<2, TestType> ddt2(2.0, 1);
    testClass<TestType>(ddt1, ddt2);
}