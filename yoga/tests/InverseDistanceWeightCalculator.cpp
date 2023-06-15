#include <RingAssertions.h>
#include <parfait/Point.h>
#include <array>
#include "InterpolationTools.h"

TEST_CASE("Calculate weights based on distance from query point") {
    std::array<Parfait::Point<double>, 4> tet;
    tet[0] = {0, 0, 0};
    tet[1] = {1, 0, 0};
    tet[2] = {0, 1, 0};
    tet[3] = {0, 0, 1};

    std::array<double, 4> weights;
    Parfait::Point<double> query_point;

    SECTION("Test on vertex, where distance will be zero") {
        query_point = {0, 0, 1};
        YOGA::FiniteElementInterpolation<double>::calcInverseDistanceWeights(
            4, tet.front().data(), query_point.data(), weights.data());

        REQUIRE(1.0 == Approx(weights[3]));
    }

    SECTION("Test in the middle of the tetrahedron") {
        query_point = {.5, .5, .5};
        YOGA::FiniteElementInterpolation<double>::calcInverseDistanceWeights(
            4, tet.front().data(), query_point.data(), weights.data());

        REQUIRE(.25 == Approx(weights[0]));
        REQUIRE(.25 == Approx(weights[1]));
        REQUIRE(.25 == Approx(weights[2]));
        REQUIRE(.25 == Approx(weights[3]));
    }
}

TEST_CASE("Make sure it works when all vertices are really close") {
    std::array<Parfait::Point<double>, 4> tet;
    double tiny = 1.0e-14;
    tet[0] = {0, 0, 0};
    tet[1] = {tiny, 0, 0};
    tet[2] = {0, tiny, 0};
    tet[3] = {0, 0, tiny};

    std::array<double, 4> weights;
    Parfait::Point<double> query_point;

    SECTION("Test on vertex, where distance will be zero") {
        query_point = {0, 0, tiny};
        YOGA::FiniteElementInterpolation<double>::calcInverseDistanceWeights(
            4, tet.front().data(), query_point.data(), weights.data());

        REQUIRE(1.0 == Approx(weights[3]));
    }
}
