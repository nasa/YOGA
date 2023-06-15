#include <RingAssertions.h>
#include <ddata/SmoothFunctions.h>

TEST_CASE("Smooth operators") {
    using namespace Linearize;
    double a = 2.0;
    double b = 0.1;
    SECTION("abs") {
        REQUIRE(a == Approx(Linearize::Smooth::abs(-a)));
        REQUIRE(a == Approx(Linearize::Smooth::abs(a)));
        REQUIRE(0.5e-6 == Approx(Linearize::Smooth::abs(0.0)));
    }
    SECTION("min") {
        REQUIRE(b == Approx(Smooth::min(a, b)));
        REQUIRE(b == Approx(Smooth::min(b, a)));
        REQUIRE(-a == Approx(Smooth::min(-a, b)));
        REQUIRE(-a == Approx(Smooth::min(b, -a)));
        REQUIRE(0.0 == Approx(Smooth::min(0.0, a)));
    }
    SECTION("max") {
        REQUIRE(a == Approx(Smooth::max(a, b)));
        REQUIRE(a == Approx(Smooth::max(b, a)));
        REQUIRE(b == Approx(Smooth::max(-a, b)));
        REQUIRE(b == Approx(Smooth::max(b, -a)));
        REQUIRE(0.0 == Approx(Smooth::max(-a, 0.0)));
    }
    SECTION("step") {
        REQUIRE(1.0 == Approx(Smooth::smooth_step(0.1)));
        REQUIRE(0.0 == Approx(Smooth::smooth_step(-0.1)));
        REQUIRE(0.5 == Approx(Smooth::smooth_step(0.0)));
    }
}
