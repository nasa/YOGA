#include <RingAssertions.h>
#include <ddata/ETD.h>
#include <YogaTypeSupport.h>
#include "InterpolationTools.hpp"

using namespace YOGA;

TEST_CASE("sanity checks on rst coords"){
    SECTION("values between [0,1] are acceptable"){
        Parfait::Point<double> rst{1, 0, 1};
        REQUIRE_FALSE(areRstCoordinatesBonkers(rst));
    }
    SECTION("values greater than 1 are bad") {
        Parfait::Point<double> rst{1, 0, 2};
        REQUIRE(areRstCoordinatesBonkers(rst));
    }
    SECTION("negative values are bad") {
        Parfait::Point<double> rst{1, 0, -.5};
        REQUIRE(areRstCoordinatesBonkers(rst));
    }
    SECTION("infs are bad") {
        double inf = 1.0/0.0;
        Parfait::Point<double> rst{inf, 0, 0};
        REQUIRE(areRstCoordinatesBonkers(rst));
    }
    SECTION("NaNs are bad") {
        double nan = std::sqrt(-1.0);
        Parfait::Point<double> rst{nan, 0, 0};
        REQUIRE(areRstCoordinatesBonkers(rst));
    }

}
