#include <parfait/Point.h>
#include <RingAssertions.h>
#include "NanoFlannDistanceCalculator.h"

using namespace YOGA;

TEST_CASE("Check that nanoflann wrapper calls it correctly") {
    using namespace nanoflann;

    std::vector<std::vector<Parfait::Point<double>>> surfaces;
    surfaces.push_back({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}});

    NanoFlannDistanceCalculator distance_calculator(surfaces);

    std::vector<Parfait::Point<double>> query_pts;
    query_pts.push_back({0, 0, 0.5});
    query_pts.push_back({1, 0, -.75});
    query_pts.push_back({0, 1, 0.33});

    std::vector<int> grid_ids{0, 0, 0};

    auto distances = distance_calculator.calculateDistances(query_pts, grid_ids);

    REQUIRE(0.5 == Approx(distances[0]));
}
