#include <RingAssertions.h>
#include <vector>
#include <parfait/Interpolation.h>

template <typename T>
inline T linearTestFunction(const Parfait::Point<T>& p) {
    return 2.3 * p[0] + 9.2 * p[1] + 3.9 * p[2] + 1.2;
}

TEST_CASE("4 points, calculate the coefficients that define a plane") {
    std::vector<Parfait::Point<double>> points;
    std::vector<double> solution;

    points.push_back({0, 0, 0});
    points.push_back({1, 0, 0});
    points.push_back({1, 1, 0});
    points.push_back({0, 0, 1});
    for (auto& p : points) solution.push_back(linearTestFunction(p));

    auto coefficients = Parfait::leastSquaresPlaneCoefficients(points.size(), points.data()->data(), solution.data());

    REQUIRE(4 == coefficients.size());

    REQUIRE(2.3 == Approx(coefficients[0]));
    REQUIRE(9.2 == Approx(coefficients[1]));
    REQUIRE(3.9 == Approx(coefficients[2]));
    REQUIRE(1.2 == Approx(coefficients[3]));
}

TEST_CASE("Given 7 points, calculate the coefficients of the plane") {
    std::vector<Parfait::Point<double>> points;
    std::vector<double> solution;
    points.push_back({0, 0, 0});
    points.push_back({1, 0, 0});
    points.push_back({1, 1, 0});
    points.push_back({0, 0, 1});
    points.push_back({2.4, 0.4, 1.9});
    points.push_back({9.6, 4.3, 9.7});
    points.push_back({1.2, 2.2, 3.2});
    for (auto& p : points) solution.push_back(linearTestFunction(p));

    Parfait::Point<double> p{0.7, 0.9, 0.3};
    double interpolated_value = Parfait::interpolate(points.size(), points.data()->data(), solution.data(), p.data());
    double answer = linearTestFunction(p);
    REQUIRE(interpolated_value == Approx(answer));
}

TEST_CASE("Can interpolate ") {
    std::vector<Parfait::Point<double>> points;
    points.push_back({0, 0, 0});
    points.push_back({1, 0, 0});
    points.push_back({1, 1, 0});
    points.push_back({0, 1, 0});
    points.push_back({0, 0, 1});
    points.push_back({1, 0, 1});
    points.push_back({1, 1, 1});
    points.push_back({0, 1, 1});

    auto num_stencil_points = (int)points.size();
    Parfait::Point<double> query = {2.7, 2.7, 2.7};
    std::vector<double> solution;
    std::vector<double> x, y, z;
    for (auto& p : points) {
        solution.push_back(linearTestFunction(p));
        x.push_back(p[0]);
        y.push_back(p[1]);
        z.push_back(p[2]);
    }

    auto interpolated_value =
        Parfait::interpolate(num_stencil_points, (double*)points.data(), solution.data(), query.data());
    REQUIRE(interpolated_value == Approx(linearTestFunction(query)));

    double interp_x = Parfait::interpolate_strict(num_stencil_points, (double*)points.data(), x.data(), query.data());
    double interp_y = Parfait::interpolate_strict(num_stencil_points, (double*)points.data(), y.data(), query.data());
    double interp_z = Parfait::interpolate_strict(num_stencil_points, (double*)points.data(), z.data(), query.data());

    // ensure the geometric interpretation of the clipped interpolation weights
    // is an interpolation inside the unit cube.
    REQUIRE((interp_x >= 0.0 and interp_x <= 1.0));
    REQUIRE((interp_y >= 0.0 and interp_y <= 1.0));
    REQUIRE((interp_z >= 0.0 and interp_z <= 1.0));
}
