#include <RingAssertions.h>

#include <parfait/Barycentric.h>

TEST_CASE("Barycentric tri works at corners") {
    SECTION("first corner") {
        Parfait::Point<double> a{0, 0, 0};
        Parfait::Point<double> b{1, 0, 0};
        Parfait::Point<double> c{0, 1, 0};
        auto weights = Parfait::calcBaryCentricWeights(a, b, c, a);
        REQUIRE(weights[0] == Approx(1.0));
        REQUIRE(weights[1] == Approx(0.0));
        REQUIRE(weights[2] == Approx(0.0));
    }
    SECTION("second corner") {
        Parfait::Point<double> a{0, 0, 0};
        Parfait::Point<double> b{1, 0, 0};
        Parfait::Point<double> c{0, 1, 0};
        auto weights = Parfait::calcBaryCentricWeights(a, b, c, b);
        REQUIRE(weights[0] == Approx(0.0));
        REQUIRE(weights[1] == Approx(1.0));
        REQUIRE(weights[2] == Approx(0.0));
    }
    SECTION("third corner") {
        Parfait::Point<double> a{0, 0, 0};
        Parfait::Point<double> b{1, 0, 0};
        Parfait::Point<double> c{0, 1, 0};
        auto weights = Parfait::calcBaryCentricWeights(a, b, c, c);
        REQUIRE(weights[0] == Approx(0.0));
        REQUIRE(weights[1] == Approx(0.0));
        REQUIRE(weights[2] == Approx(1.0));
    }
}

TEST_CASE("Barycentric tri works at edge") {
    SECTION("first edge") {
        Parfait::Point<double> a{0, 0, 0};
        Parfait::Point<double> b{1, 0, 0};
        Parfait::Point<double> c{0, 1, 0};
        auto weights = Parfait::calcBaryCentricWeights(a, b, c, 0.5 * (a + b));
        REQUIRE(weights[0] == Approx(0.5));
        REQUIRE(weights[1] == Approx(0.5));
        REQUIRE(weights[2] == Approx(0.0));
    }
    SECTION("second edge") {
        Parfait::Point<double> a{0, 0, 0};
        Parfait::Point<double> b{1, 0, 0};
        Parfait::Point<double> c{0, 1, 0};
        auto weights = Parfait::calcBaryCentricWeights(a, b, c, 0.5 * (b + c));
        REQUIRE(weights[0] == Approx(0.0));
        REQUIRE(weights[1] == Approx(0.5));
        REQUIRE(weights[2] == Approx(0.5));
    }

    SECTION("third edge") {
        Parfait::Point<double> a{0, 0, 0};
        Parfait::Point<double> b{1, 0, 0};
        Parfait::Point<double> c{0, 1, 0};
        auto weights = Parfait::calcBaryCentricWeights(a, b, c, 0.5 * (a + c));
        REQUIRE(weights[0] == Approx(0.5));
        REQUIRE(weights[1] == Approx(0.0));
        REQUIRE(weights[2] == Approx(0.5));
    }
}
TEST_CASE("Barycentric tri works inside the area") {
    double third = 1.0 / 3.0;
    Parfait::Point<double> a{0, 0, 0};
    Parfait::Point<double> b{1, 0, 0};
    Parfait::Point<double> c{0, 1, 0};
    auto weights = Parfait::calcBaryCentricWeights(a, b, c, third * (a + b + c));
    REQUIRE(weights[0] == Approx(third));
    REQUIRE(weights[1] == Approx(third));
    REQUIRE(weights[2] == Approx(third));
}

SCENARIO("Barycentric tet coordinate transformation", "") {
    std::array<Parfait::Point<double>, 4> tet;
    tet[0] = {0, 0, 0};
    tet[1] = {3, 1, 0};
    tet[2] = {1, 4, 0};
    tet[3] = {0, 1, 2};
    GIVEN("a tet and a point at vertex 0, the barycentric coords are 1,0,0,0") {
        Parfait::Point<double> p(0, 0, 0);
        auto bary = Parfait::calculateBarycentricCoordinates(tet, p);

        REQUIRE(1 == Approx(bary[0]));
        REQUIRE(0 == Approx(bary[1]));
        REQUIRE(0 == Approx(bary[2]));
        REQUIRE(0 == Approx(bary[3]));
    }

    GIVEN("a tet and a point at vertex 3, the barycentric coords are 0,1,0,0") {
        Parfait::Point<double> p(0, 1, 2);
        auto bary = Parfait::calculateBarycentricCoordinates(tet, p);

        REQUIRE(0 == Approx(bary[0]));
        REQUIRE(0 == Approx(bary[1]));
        REQUIRE(0 == Approx(bary[2]));
        REQUIRE(1 == Approx(bary[3]));
    }

    GIVEN("a tet and the midpoint of vertex 0 & 1, the barycentric coords are .5,.5,0,0") {
        Parfait::Point<double> p(1.5, 0.5, 0);
        auto bary = Parfait::calculateBarycentricCoordinates(tet, p);

        REQUIRE(.5 == Approx(bary[0]));
        REQUIRE(.5 == Approx(bary[1]));
        REQUIRE(0 == Approx(bary[2]));
        REQUIRE(0 == Approx(bary[3]));
    }

    GIVEN("a particular tet and applesauce") {
        Parfait::Point<double> p = {7.249069, -2.531457, -1.839211};
        std::array<Parfait::Point<double>, 4> problemTet;
        problemTet[0] = {7.245265, -2.533199, -1.840477};
        problemTet[1] = {7.243145, -2.529665, -1.837909};
        problemTet[2] = {7.259916, -2.524502, -1.834158};
        problemTet[3] = {7.262680, -2.527456, -1.836304};
        auto bary = Parfait::calculateBarycentricCoordinates(problemTet, p);
        Parfait::Point<double> interpolatedXyz{0, 0, 0};
        for (int i = 0; i < 4; ++i) interpolatedXyz += bary[i] * problemTet[i];

        REQUIRE(p[0] == Approx(interpolatedXyz[0]));
        REQUIRE(p[1] == Approx(interpolatedXyz[1]));
        REQUIRE(p[2] == Approx(interpolatedXyz[2]));
    }
}
