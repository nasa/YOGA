#include "InterpolationTools.h"
#include <RingAssertions.h>

using Parfait::Point;
using std::array;
using namespace YOGA;

SCENARIO("Barycentric coordinate transformation") {
    std::array<Parfait::Point<double>, 4> tet;
    tet[0] = {0, 0, 0};
    tet[1] = {3, 1, 0};
    tet[2] = {1, 4, 0};
    tet[3] = {0, 1, 2};
    GIVEN("a tet and a point at vertex 0, the barycentric coords are 1,0,0,0") {
        Point<double> p(0, 0, 0);
        auto bary = BarycentricInterpolation::calculateBarycentricCoordinates(tet, p);

        REQUIRE(1 == Approx(bary[0]));
        REQUIRE(0 == Approx(bary[1]));
        REQUIRE(0 == Approx(bary[2]));
        REQUIRE(0 == Approx(bary[3]));
    }

    GIVEN("a tet and a point at vertex 3, the barycentric coords are 0,1,0,0") {
        Point<double> p(0, 1, 2);
        auto bary = BarycentricInterpolation::calculateBarycentricCoordinates(tet, p);

        REQUIRE(0 == Approx(bary[0]));
        REQUIRE(0 == Approx(bary[1]));
        REQUIRE(0 == Approx(bary[2]));
        REQUIRE(1 == Approx(bary[3]));
    }

    GIVEN("a tet and the midpoint of vertex 0 & 1, the barycentric coords are .5,.5,0,0") {
        Point<double> p(1.5, 0.5, 0);
        auto bary = BarycentricInterpolation::calculateBarycentricCoordinates(tet, p);

        REQUIRE(.5 == Approx(bary[0]));
        REQUIRE(.5 == Approx(bary[1]));
        REQUIRE(0 == Approx(bary[2]));
        REQUIRE(0 == Approx(bary[3]));
    }

    GIVEN("a particular tet and applesauce") {
        Point<double> p = {7.249069, -2.531457, -1.839211};
        std::array<Parfait::Point<double>, 4> problemTet;
        problemTet[0] = {7.245265, -2.533199, -1.840477};
        problemTet[1] = {7.243145, -2.529665, -1.837909};
        problemTet[2] = {7.259916, -2.524502, -1.834158};
        problemTet[3] = {7.262680, -2.527456, -1.836304};
        auto bary = BarycentricInterpolation::calculateBarycentricCoordinates(problemTet, p);
        Parfait::Point<double> interpolatedXyz{0, 0, 0};
        for (int i = 0; i < 4; ++i) interpolatedXyz += bary[i] * problemTet[i];

        REQUIRE(p[0] == Approx(interpolatedXyz[0]));
        REQUIRE(p[1] == Approx(interpolatedXyz[1]));
        REQUIRE(p[2] == Approx(interpolatedXyz[2]));
    }
}
