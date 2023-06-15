#include <parfait/Point.h>
#include "SymmetryPlane.h"
#include <RingAssertions.h>
using Parfait::Point;
using namespace YOGA;
SCENARIO("Check that symmetry planes work") {
    GIVEN("a plane associated with component 2, with symmetry about x=2.3") {
        SymmetryPlane plane(2, 0, 2.3);
        REQUIRE(2 == plane.componentId());

        THEN("Reflect a Point<double> about the plane") {
            Point<double> p(4, 2, 3);
            plane.reflect(p);
            REQUIRE(0.6 == Approx(p[0]));
            REQUIRE(2 == Approx(p[1]));
            REQUIRE(3 == Approx(p[2]));
        }
        THEN("Do the same thing for a point of different type") {
            double p[3] = {4, 2, 3};
            plane.reflect(p);
            REQUIRE(0.6 == Approx(p[0]));
            REQUIRE(2 == Approx(p[1]));
            REQUIRE(3 == Approx(p[2]));
        }
    }
}
