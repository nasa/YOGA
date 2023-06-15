#include <parfait/Point.h>
#include <vector>
#include "InterpolationTools.h"
#include <RingAssertions.h>
#include "linear_test_function.h"

using std::array;
using std::vector;
using namespace Parfait;

class MockTet {
  public:
    MockTet() {
        vertices[0] = {0, 0, 0};
        vertices[1] = {3, 1, 0};
        vertices[2] = {1, 4, 0};
        vertices[3] = {0, 1, 2};
    }

    const Point<double>& operator[](int i) const { return vertices[i]; }

  private:
    array<Point<double>, 4> vertices;
};

double interpolate(array<double, 4>& solution, array<double, 4>& weights) {
    return solution[0] * weights[0] + solution[1] * weights[1] + solution[2] * weights[2] + solution[3] * weights[3];
}
using namespace YOGA;
SCENARIO("To interpolate in a tetrahedron") {
    std::array<Parfait::Point<double>, 4> tet;
    tet[0] = {0, 0, 0};
    tet[1] = {3, 1, 0};
    tet[2] = {1, 4, 0};
    tet[3] = {0, 1, 2};

    array<double, 4> solution;
    for (int i = 0; i < 4; i++) solution[i] = linearTestFunction(tet[i]);
    GIVEN("a tet and a point, calc interpolation weights") {
        // the point is vertex 0, so the interpolated
        // solution should be the same as the solution at
        // that point
        Point<double> p = tet[0];
        std::array<double,4> weights;
        BarycentricInterpolation::calcWeightsTet(tet.front().data(), p.data(), weights.data());
        double expected = solution[0];
        double actual = interpolate(solution, weights);
        REQUIRE(expected == Approx(actual));
    }

    GIVEN("a tet and a point, calculate the weights") {
        // the point is inside the tet and the
        // test function is linear, so we should
        // get exactly the right answer
        Point<double> p(1, 1.5, .5);
        std::array<double,4> weights;
        BarycentricInterpolation::calcWeightsTet(tet.front().data(), p.data(), weights.data());
        double expected = linearTestFunction(p);
        double actual = interpolate(solution, weights);
        REQUIRE(expected == Approx(actual));
    }
}
