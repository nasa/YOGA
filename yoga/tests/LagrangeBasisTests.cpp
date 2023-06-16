#include <ddata/ETD.h>
#include <YogaTypeSupport.h>
#include <parfait/Point.h>
#include <parfait/CellContainmentChecker.h>
#include <RingAssertions.h>
#include "InterpolationTools.h"
#include "LagrangeElement.h"
#include "linear_test_function.h"

using namespace YOGA;

TEST_CASE("Lagrange basis for Tetrahedron") {
    Parfait::Point<double> p1 = {0, 0, 0};
    Parfait::Point<double> p2 = {2, 0, 0};
    Parfait::Point<double> p3 = {0, 2, 0};
    Parfait::Point<double> p4 = {0, 0, 2};
    LagrangeTet<double> tet({p1, p2, p3, p4});

    REQUIRE(tet.evaluate(0, 0, 0) == p1);
    REQUIRE(tet.evaluate(1, 0, 0) == p2);
    REQUIRE(tet.evaluate(0, 1, 0) == p3);
    REQUIRE(tet.evaluate(0, 0, 1) == p4);

    REQUIRE(tet.calcComputationalCoordinates({0, 0, 0}) == Parfait::Point<double>{0, 0, 0});
    REQUIRE(tet.calcComputationalCoordinates({2, 0, 0}) == Parfait::Point<double>{1, 0, 0});
    REQUIRE(tet.calcComputationalCoordinates({0, 2, 0}) == Parfait::Point<double>{0, 1, 0});
    REQUIRE(tet.calcComputationalCoordinates({0, 0, 2}) == Parfait::Point<double>{0, 0, 1});

    SECTION("interpolate a field") {
        std::array<Parfait::Point<double>, 4> vertices = {p1, p2, p3, p4};
        Parfait::Point<double> p = {.1, .1, .1};
        std::array<double, 4> weights;
        FiniteElementInterpolation<double>::calcWeightsTet(vertices.front().data(), p.data(), weights.data());
        double sum = 0;
        for (size_t i = 0; i < weights.size(); i++) {
            sum += weights[i] * linearTestFunction(vertices[i]);
        }
        REQUIRE(linearTestFunction(p) == Approx(sum));
    }
}

template <typename Point>
bool areApproxEqual(Point a, Point b) {
    for (int i = 0; i < 3; i++) {
        if (std::abs(b[i].value - a[i].value) > 1.0e-15) return false;
    }
    return true;
}

TEST_CASE("Lagrange basis for the worst element type (pyramid)") {
    Parfait::Point<double> p1 = {0, 0, 0};
    Parfait::Point<double> p2 = {2, 0, 0};
    Parfait::Point<double> p3 = {2, 2, 0};
    Parfait::Point<double> p4 = {0, 2, 0};
    Parfait::Point<double> p5 = {1, 1, 2};
    LagrangePyramid<double> pyramid({p1, p2, p3, p4, p5});

    REQUIRE(pyramid.evaluate(0, 0, 0) == p1);
    REQUIRE(pyramid.evaluate(1, 0, 0) == p2);
    REQUIRE(pyramid.evaluate(1, 1, 0) == p3);
    REQUIRE(pyramid.evaluate(0, 1, 0) == p4);
    REQUIRE(pyramid.evaluate(0, 0, 1) == p5);

    REQUIRE(pyramid.calcComputationalCoordinates({0, 0, 0}).approxEqual(Parfait::Point<double>{0, 0, 0}));
    REQUIRE(pyramid.calcComputationalCoordinates({2, 0, 0}).approxEqual(Parfait::Point<double>{1, 0, 0}));
    REQUIRE(pyramid.calcComputationalCoordinates({2, 2, 0}).approxEqual(Parfait::Point<double>{1, 1, 0}));
    REQUIRE(pyramid.calcComputationalCoordinates({0, 2, 0}).approxEqual(Parfait::Point<double>{0, 1, 0}));
    REQUIRE(pyramid.calcComputationalCoordinates({1, 1, 2})[2] == 1);

    SECTION("interpolate a field") {
        std::array<Parfait::Point<double>, 5> vertices = {p1, p2, p3, p4, p5};
        Parfait::Point<double> p = {.1, .1, .1};
        std::array<double, 5> weights;
        FiniteElementInterpolation<double>::calcWeightsPyramid(vertices.front().data(), p.data(), weights.data());
        double sum = 0;
        for (size_t i = 0; i < weights.size(); i++) {
            sum += weights[i] * linearTestFunction(vertices[i]);
        }
        REQUIRE(linearTestFunction(p) == Approx(sum));
    }
}

TEST_CASE("Special pyramid"){
    typedef Parfait::Point<double> Point;

    Point v0 = {957.164938, -134.405743, 100.000093};
    Point v1 = {957.174424, -134.444146, 100.073089};
    Point v2 = {957.206344, -134.408242, 100.077366};
    Point v3 = {957.191724, -134.359330, 100.000129};
    Point v4 = {957.129268, -134.402117, 100.045373};

    std::array<Parfait::Point<double>,5> vertices = {v0,v1,v2,v3,v4};
    //vertices = {v0,v1,v2,v3,v4};
    //vertices = {v3,v0,v1,v2,v4};
    //vertices = {v2,v3,v0,v1,v4};
    vertices = {v1,v2,v3,v0,v4};
    LagrangePyramid<double> pyramid(vertices);

    Parfait::Point<double> query_point = {957.187953, -134.362668, 100.003180};

    bool is_in = Parfait::CellContainmentChecker::isInCell<5>(vertices, query_point);
    REQUIRE(is_in);

    std::array<double,5> weights;
    FiniteElementInterpolation<double>::calcWeightsPyramid(vertices.front().data(), query_point.data(), weights.data());

    double sum = 0;
    for (size_t i = 0; i < weights.size(); i++) {
        sum += weights[i] * linearTestFunction(vertices[i]);
    }

    double expected = linearTestFunction(query_point);
    double tol = expected * 1e-4;
    REQUIRE(linearTestFunction(query_point) == Approx(sum).margin(tol));
}

TEST_CASE("Lagrange basis for prisms") {
    Parfait::Point<double> p1 = {0, 0, 0};
    Parfait::Point<double> p2 = {2, 0, 0};
    Parfait::Point<double> p3 = {0, 2, 0};
    Parfait::Point<double> p4 = {0, 0, 3};
    Parfait::Point<double> p5 = {2, 0, 3};
    Parfait::Point<double> p6 = {0, 2, 3};
    LagrangePrism<double> prism({p1, p2, p3, p4, p5, p6});

    REQUIRE(prism.evaluate(0, 0, 0) == p1);
    REQUIRE(prism.evaluate(1, 0, 0) == p2);
    REQUIRE(prism.evaluate(0, 1, 0) == p3);
    REQUIRE(prism.evaluate(0, 0, 1) == p4);
    REQUIRE(prism.evaluate(1, 0, 1) == p5);
    REQUIRE(prism.evaluate(0, 1, 1) == p6);

    REQUIRE(prism.calcComputationalCoordinates({0, 0, 0}) == Parfait::Point<double>{0, 0, 0});
    REQUIRE(prism.calcComputationalCoordinates({2, 0, 0}) == Parfait::Point<double>{1, 0, 0});
    REQUIRE(prism.calcComputationalCoordinates({0, 2, 0}) == Parfait::Point<double>{0, 1, 0});
    REQUIRE(prism.calcComputationalCoordinates({0, 0, 3}) == Parfait::Point<double>{0, 0, 1});
    REQUIRE(prism.calcComputationalCoordinates({2, 0, 3}) == Parfait::Point<double>{1, 0, 1});
    REQUIRE(prism.calcComputationalCoordinates({0, 2, 3}) == Parfait::Point<double>{0, 1, 1});

    SECTION("interpolate a field") {
        std::array<Parfait::Point<double>, 6> vertices = {p1, p2, p3, p4, p5, p6};
        Parfait::Point<double> p = {.1, .1, .1};
        std::array<double, 6> weights;
        FiniteElementInterpolation<double>::calcWeightsPrism(vertices.front().data(), p.data(), weights.data());
        double sum = 0;
        for (size_t i = 0; i < weights.size(); i++) {
            sum += weights[i] * linearTestFunction(vertices[i]);
        }
        REQUIRE(linearTestFunction(p) == Approx(sum));
    }
}

TEST_CASE("Strange prism extracted from fun3d  case") {
    std::array<Parfait::Point<double>, 6> vertices;
    vertices[0] = {5.440580e-01, 8.609290e-02, 6.935788e-01};
    vertices[1] = {5.284717e-01, 8.540347e-02, 6.925709e-01};
    vertices[2] = {5.374334e-01, 7.340075e-02, 6.933101e-01};
    vertices[3] = {5.407550e-01, 8.725931e-02, 6.980190e-01};
    vertices[4] = {5.247318e-01, 8.665488e-02, 6.966246e-01};
    vertices[5] = {5.339565e-01, 7.444268e-02, 6.976473e-01};
    LagrangePrism<double> prism(vertices);

    Parfait::Point<double> query_point{5.323858e-01, 8.542856e-02, 6.944939e-01};

    bool is_in = Parfait::CellContainmentChecker::isInCell<6>(vertices, query_point);
    REQUIRE(is_in);

    auto rst = prism.calcComputationalCoordinates(query_point);
    auto guess = prism.evaluate(rst[0], rst[1], rst[2]);

    REQUIRE(query_point[0] == Approx(guess[0]));
    REQUIRE(query_point[1] == Approx(guess[1]));
    REQUIRE(query_point[2] == Approx(guess[2]));
}

TEST_CASE("Lagrange basis for hexs") {
    Parfait::Point<double> p1 = {0, 0, 0};
    Parfait::Point<double> p2 = {2, 0, 0};
    Parfait::Point<double> p3 = {2, 2, 0};
    Parfait::Point<double> p4 = {0, 2, 0};
    Parfait::Point<double> p5 = {0, 0, 3};
    Parfait::Point<double> p6 = {2, 0, 3};
    Parfait::Point<double> p7 = {2, 2, 3};
    Parfait::Point<double> p8 = {0, 2, 3};
    LagrangeHex<double> hex({p1, p2, p3, p4, p5, p6, p7, p8});

    REQUIRE(hex.evaluate(0, 0, 0) == p1);
    REQUIRE(hex.evaluate(1, 0, 0) == p2);
    REQUIRE(hex.evaluate(1, 1, 0) == p3);
    REQUIRE(hex.evaluate(0, 1, 0) == p4);
    REQUIRE(hex.evaluate(0, 0, 1) == p5);
    REQUIRE(hex.evaluate(1, 0, 1) == p6);
    REQUIRE(hex.evaluate(1, 1, 1) == p7);
    REQUIRE(hex.evaluate(0, 1, 1) == p8);

    double tol = 1.0e-12;
    REQUIRE(hex.calcComputationalCoordinates({0, 0, 0}).approxEqual({0, 0, 0}, tol));
    REQUIRE(hex.calcComputationalCoordinates({2, 0, 0}).approxEqual({1, 0, 0}, tol));
    REQUIRE(hex.calcComputationalCoordinates({2, 2, 0}).approxEqual({1, 1, 0}, tol));
    REQUIRE(hex.calcComputationalCoordinates({0, 2, 0}).approxEqual({0, 1, 0}, tol));
    REQUIRE(hex.calcComputationalCoordinates({0, 0, 3}).approxEqual({0, 0, 1}, tol));
    REQUIRE(hex.calcComputationalCoordinates({2, 0, 3}).approxEqual({1, 0, 1}, tol));
    REQUIRE(hex.calcComputationalCoordinates({2, 2, 3}).approxEqual({1, 1, 1}, tol));
    REQUIRE(hex.calcComputationalCoordinates({0, 2, 3}).approxEqual({0, 1, 1}, tol));

    SECTION("interpolate a field") {
        std::array<Parfait::Point<double>, 8> vertices = {p1, p2, p3, p4, p5, p6, p7, p8};
        Parfait::Point<double> p = {.1, .1, .1};
        std::array<double, 8> weights;
        FiniteElementInterpolation<double>::calcWeightsHex(vertices.front().data(), p.data(), weights.data());
        double sum = 0;
        for (size_t i = 0; i < weights.size(); i++) {
            sum += weights[i] * linearTestFunction(vertices[i]);
        }
        REQUIRE(linearTestFunction(p) == Approx(sum));
    }
}
