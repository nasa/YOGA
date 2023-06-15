#include <RingAssertions.h>
#include "parfait/SCurve.h"

TEST_CASE("S curve") {
    SECTION("Basic curve from 0 to 1 transitioning at x = 0") {
        auto s_curve = Parfait::SCurve::create();
        REQUIRE(s_curve(-100) == Approx(0.0).margin(1e-8));
        REQUIRE(s_curve(100) == Approx(1.0));
    }
    SECTION("Steep curve at x = 0") {
        auto s_curve = Parfait::SCurve::create(0, 1, 0, 10);
        REQUIRE(s_curve(-1) == Approx(0.0).margin(1e-2));
        REQUIRE(s_curve(1) == Approx(1.0).margin(1e-2));
    }
    SECTION("Steep curve at x = 5") {
        auto s_curve = Parfait::SCurve::create(0, 1, 5, 10);
        REQUIRE(s_curve(4) == Approx(0.0).margin(1e-2));
        REQUIRE(s_curve(6) == Approx(1.0).margin(1e-2));
    }
    SECTION("Steep curve between 2 and 6 transitioning at x = 5") {
        auto s_curve = Parfait::SCurve::create(2, 6, 5, 20);
        REQUIRE(s_curve(4) == Approx(2.0).margin(1e-2));
        REQUIRE(s_curve(6) == Approx(6.0).margin(1e-2));
    }
}