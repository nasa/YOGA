#include <parfait/Point.h>
#include <vector>
#include "InterpolationTools.h"
#include <RingAssertions.h>
#include "linear_test_function.h"
#include <parfait/LeastSquaresReconstruction.h>

using Parfait::Point;
using namespace Parfait;
using std::vector;

SCENARIO("Least squares interpolation") {
    vector<Point<double>> points;

    GIVEN("Given 7 points, calculate the coefficients of the plane") {
        points.push_back({0, 0, 0});
        points.push_back({1, 0, 0});
        points.push_back({1, 1, 0});
        points.push_back({0, 0, 1});
        points.push_back({2.4, 0.4, 1.9});
        points.push_back({9.6, 4.3, 9.7});
        points.push_back({1.2, 2.2, 3.2});

        Parfait::Point<double>  query_point {0.7, 0.9, 0.3};
        int n = points.size();
        std::vector<double> weights(n);

        YOGA::LeastSquaresInterpolation<double>::calcWeights(n,points.front().data(),query_point.data(),weights.data());
        //YOGA::FiniteElementInterpolation<double>::calcWeights(n,points.front().data(),query_point.data(),weights.data());


        WHEN("interpolating the linear field") {
            double interpolated_value = 0.0;
            for(int i=0;i<n;i++)
                interpolated_value += weights[i] * linearTestFunction(points[i]);
            double answer = linearTestFunction(query_point);
            REQUIRE(interpolated_value == Approx(answer));
        }

    }
#if 0
    SECTION("problem cell"){
        points.push_back({0.7219, 1.73066, 0.813134});
        points.push_back({0.730011, 1.71879, 0.821126});
        points.push_back({0.717929, 1.73471, 0.822558});
        points.push_back({0.715618, 1.72675, 0.821163});

        //Projected point: -2.02134, 3.29915, 1.09346
        Parfait::Point<double> receptor = {0.717368, 1.72894, 0.819935};
        int n = points.size();
        std::vector<double> weights;
        weights.push_back(-28.0649);
        weights.push_back(-182.523);
        weights.push_back(28.7358);
        weights.push_back(182.852);
        //YOGA::LeastSquaresInterpolation<double>::calcWeights(n,points.front().data(),receptor.data(),weights.data());

        Parfait::Point<double> interpolated_xyz {0,0,0};
        for(int i=0;i<n;i++)
            interpolated_xyz += weights[i] * points[i];
        printf("Interpolated: %s\n",interpolated_xyz.to_string().c_str());
        printf("Receptor:     %s\n",receptor.to_string().c_str());
        REQUIRE(receptor.approxEqual(interpolated_xyz));
    }
#endif
}
