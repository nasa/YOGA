#include <RingAssertions.h>
#include <vector>
#include <parfait/LeastSquaresReconstruction.h>
#include <parfait/Interpolation.h>

double trilinearTestFunction(double x, double y, double z) { return 10.0 * x + 3.0 * y + -6.6 * z; }
double sphereTestFunction(double x, double y, double z) { return 5.0 * x * x + 2.0 * y * y - 7.2 * z * z; }
std::vector<Parfait::Point<double>> buildUnitCubePoints(int n_points_x, int n_points_y, int n_points_z) {
    double dx = 1.0 / double(std::max(n_points_x, 2) - 1);
    double dy = 1.0 / double(std::max(n_points_y, 2) - 1);
    double dz = 1.0 / double(std::max(n_points_z, 2) - 1);
    std::vector<Parfait::Point<double>> unit_cube_points;
    for (int i = 0; i < n_points_x; ++i) {
        for (int j = 0; j < n_points_y; ++j) {
            for (int k = 0; k < n_points_z; ++k) {
                Parfait::Point<double> p(i * dx, j * dy, k * dz);
                unit_cube_points.push_back(p);
            }
        }
    }
    return unit_cube_points;
}

TEST_CASE("LSQ coefficient calculation - 1D") {
    auto unit_cube_points = buildUnitCubePoints(2, 1, 1);
    Parfait::Point<double> center = {0.5, 0.0, 0.0};

    auto get_stencil_point = [&](int n) { return unit_cube_points[n]; };
    auto num_stencil_points = (int)unit_cube_points.size();
    double lsq_weight_power = 0.0;

    SECTION("Full LSQ") {
        auto coeffs = Parfait::calcLLSQCoefficients(
            get_stencil_point, num_stencil_points, lsq_weight_power, center, Parfait::OneD);
        REQUIRE(coeffs.rows() == 2);
        REQUIRE(coeffs.cols() == 2);

        int i = 0;
        double f = 0;
        Parfait::Point<double> grad{0, 0, 0};
        for (auto& p : unit_cube_points) {
            auto q = trilinearTestFunction(p[0], p[1], p[2]);
            f += coeffs(i, 0) * q;
            grad[0] += coeffs(i, 1) * q;
            i++;
        }

        REQUIRE(f == Approx(trilinearTestFunction(center[0], center[1], center[2])));
        REQUIRE(grad[0] == Approx(10.0));
    }
    SECTION("Gradient-only LSQ") {
        auto coeffs = Parfait::calcLLSQGradientCoefficients(
            get_stencil_point, num_stencil_points, lsq_weight_power, center, Parfait::OneD);
        REQUIRE(coeffs.rows() == 2);
        REQUIRE(coeffs.cols() == 1);

        auto center_value = trilinearTestFunction(center[0], center[1], center[2]);

        int i = 0;
        Parfait::Point<double> grad{0, 0, 0};
        for (auto& p : unit_cube_points) {
            auto dq = trilinearTestFunction(p[0], p[1], p[2]) - center_value;
            grad[0] += coeffs(i, 0) * dq;
            i++;
        }

        REQUIRE(grad[0] == Approx(10.0));
    }
}

TEST_CASE("quadratic LSQ coefficient calculation - 1D") {
    auto unit_cube_points = buildUnitCubePoints(3, 1, 1);
    Parfait::Point<double> center = {0.5, 0.0, 0.0};

    auto get_stencil_point = [&](int n) { return unit_cube_points[n]; };
    auto num_stencil_points = (int)unit_cube_points.size();
    double stencil_point_weight = 0.0;

    SECTION("Full QLSQ") {
        auto coeffs = Parfait::calcQLSQCoefficients(
            get_stencil_point, num_stencil_points, stencil_point_weight, center, Parfait::OneD);
        REQUIRE(num_stencil_points == coeffs.rows());
        REQUIRE(3 == coeffs.cols());

        int i = 0;
        double f = 0.0;
        Parfait::Point<double> grad{0, 0, 0};
        double dxx = 0;
        for (auto& p : unit_cube_points) {
            auto q = sphereTestFunction(p[0], p[1], p[2]);
            f += coeffs(i, 0) * q;
            grad[0] += coeffs(i, 1) * q;
            dxx += coeffs(i, 2) * q;
            i++;
        }

        auto center_value = sphereTestFunction(center[0], center[1], center[2]);
        CHECK(center_value == Approx(f));
        CHECK(5.0 == Approx(grad[0]));
        CHECK(10.0 == Approx(dxx));
    }
    SECTION("Only Gradient and Hessian QLSQ") {
        auto coeffs = Parfait::calcQLSQGradientHessianCoefficients(
            get_stencil_point, num_stencil_points, stencil_point_weight, center, Parfait::OneD);
        REQUIRE(num_stencil_points == coeffs.rows());
        REQUIRE(2 == coeffs.cols());

        auto center_value = sphereTestFunction(center[0], center[1], center[2]);

        int i = 0;
        double dxx = 0;
        Parfait::Point<double> grad{0, 0, 0};
        for (auto& p : unit_cube_points) {
            auto dq = sphereTestFunction(p[0], p[1], p[2]) - center_value;
            grad[0] += coeffs(i, 0) * dq;
            dxx += coeffs(i, 1) * dq;
            i++;
        }

        CHECK(5.0 == Approx(grad[0]));
        CHECK(10.0 == Approx(dxx));
    }
}

TEST_CASE("LSQ coefficient calculation - 2D") {
    auto unit_cube_points = buildUnitCubePoints(2, 2, 1);
    Parfait::Point<double> center = {0.5, 0.5, 0.0};

    auto get_stencil_point = [&](int n) { return unit_cube_points[n]; };
    auto num_stencil_points = (int)unit_cube_points.size();
    double lsq_weight_power = 0.0;

    SECTION("Full LSQ") {
        auto coeffs = Parfait::calcLLSQCoefficients(
            get_stencil_point, num_stencil_points, lsq_weight_power, center, Parfait::TwoD);
        REQUIRE(coeffs.rows() == 4);
        REQUIRE(coeffs.cols() == 3);

        int i = 0;
        double f = 0;
        Parfait::Point<double> grad{0, 0, 0};
        for (auto& p : unit_cube_points) {
            auto q = trilinearTestFunction(p[0], p[1], p[2]);
            f += coeffs(i, 0) * q;
            grad[0] += coeffs(i, 1) * q;
            grad[1] += coeffs(i, 2) * q;
            i++;
        }

        REQUIRE(f == Approx(trilinearTestFunction(center[0], center[1], center[2])));
        REQUIRE(grad[0] == Approx(10.0));
        REQUIRE(grad[1] == Approx(3.0));
    }
    SECTION("Gradient-only LSQ") {
        auto coeffs = Parfait::calcLLSQGradientCoefficients(
            get_stencil_point, num_stencil_points, lsq_weight_power, center, Parfait::TwoD);
        REQUIRE(coeffs.rows() == 4);
        REQUIRE(coeffs.cols() == 2);

        auto center_value = trilinearTestFunction(center[0], center[1], center[2]);

        int i = 0;
        Parfait::Point<double> grad{0, 0, 0};
        for (auto& p : unit_cube_points) {
            auto dq = trilinearTestFunction(p[0], p[1], p[2]) - center_value;
            grad[0] += coeffs(i, 0) * dq;
            grad[1] += coeffs(i, 1) * dq;
            i++;
        }

        REQUIRE(grad[0] == Approx(10.0));
        REQUIRE(grad[1] == Approx(3.0));
    }
}

TEST_CASE("LSQ coefficient calculation - 3D") {
    auto unit_cube_points = buildUnitCubePoints(2, 2, 2);
    Parfait::Point<double> center = {0.5, 0.5, 0.5};

    auto get_stencil_point = [&](int n) { return unit_cube_points[n]; };
    int num_stencil_points = 8;
    double lsq_weight_power = 0.0;

    SECTION("Full LSQ") {
        auto coeffs = Parfait::calcLLSQCoefficients(
            get_stencil_point, num_stencil_points, lsq_weight_power, center, Parfait::ThreeD);
        REQUIRE(coeffs.rows() == 8);
        REQUIRE(coeffs.cols() == 4);

        int i = 0;
        double f = 0;
        Parfait::Point<double> grad{0, 0, 0};
        for (auto& p : unit_cube_points) {
            auto q = trilinearTestFunction(p[0], p[1], p[2]);
            f += coeffs(i, 0) * q;
            grad[0] += coeffs(i, 1) * q;
            grad[1] += coeffs(i, 2) * q;
            grad[2] += coeffs(i, 3) * q;
            i++;
        }

        REQUIRE(f == Approx(trilinearTestFunction(center[0], center[1], center[2])));
        REQUIRE(grad[0] == Approx(10.0));
        REQUIRE(grad[1] == Approx(3.0));
        REQUIRE(grad[2] == Approx(-6.6));
    }
    SECTION("Gradient-only LSQ") {
        auto coeffs = Parfait::calcLLSQGradientCoefficients(
            get_stencil_point, num_stencil_points, lsq_weight_power, center, Parfait::ThreeD);
        REQUIRE(coeffs.rows() == 8);
        REQUIRE(coeffs.cols() == 3);

        auto center_value = trilinearTestFunction(center[0], center[1], center[2]);

        int i = 0;
        Parfait::Point<double> grad{0, 0, 0};
        for (auto& p : unit_cube_points) {
            auto dq = trilinearTestFunction(p[0], p[1], p[2]) - center_value;
            grad[0] += coeffs(i, 0) * dq;
            grad[1] += coeffs(i, 1) * dq;
            grad[2] += coeffs(i, 2) * dq;
            i++;
        }

        REQUIRE(grad[0] == Approx(10.0));
        REQUIRE(grad[1] == Approx(3.0));
        REQUIRE(grad[2] == Approx(-6.6));
    }
}

TEST_CASE("quadratic LSQ coefficient calculation - 2D") {
    auto unit_cube_points = buildUnitCubePoints(3, 3, 1);
    Parfait::Point<double> center = {0.5, 0.5, 0.0};

    auto get_stencil_point = [&](int n) { return unit_cube_points[n]; };
    auto num_stencil_points = (int)unit_cube_points.size();
    double stencil_point_weight = 0.0;

    SECTION("Full QLSQ") {
        auto coeffs = Parfait::calcQLSQCoefficients(
            get_stencil_point, num_stencil_points, stencil_point_weight, center, Parfait::TwoD);
        REQUIRE(num_stencil_points == coeffs.rows());
        REQUIRE(6 == coeffs.cols());

        int i = 0;
        double f = 0.0;
        Parfait::Point<double> grad{0, 0, 0};
        double dxx = 0;
        double dyy = 0;
        for (auto& p : unit_cube_points) {
            auto q = sphereTestFunction(p[0], p[1], p[2]);
            f += coeffs(i, 0) * q;
            grad[0] += coeffs(i, 1) * q;
            grad[1] += coeffs(i, 2) * q;
            dxx += coeffs(i, 3) * q;
            dyy += coeffs(i, 5) * q;
            i++;
        }

        auto center_value = sphereTestFunction(center[0], center[1], center[2]);
        CHECK(center_value == Approx(f));
        CHECK(5.0 == Approx(grad[0]));
        CHECK(2.0 == Approx(grad[1]));
        CHECK(10.0 == Approx(dxx));
        CHECK(4.0 == Approx(dyy));
    }
    SECTION("Only Gradient and Hessian QLSQ") {
        auto coeffs = Parfait::calcQLSQGradientHessianCoefficients(
            get_stencil_point, num_stencil_points, stencil_point_weight, center, Parfait::TwoD);
        REQUIRE(num_stencil_points == coeffs.rows());
        REQUIRE(5 == coeffs.cols());

        auto center_value = sphereTestFunction(center[0], center[1], center[2]);

        int i = 0;
        double dxx = 0;
        double dyy = 0;
        Parfait::Point<double> grad{0, 0, 0};
        for (auto& p : unit_cube_points) {
            auto dq = sphereTestFunction(p[0], p[1], p[2]) - center_value;
            grad[0] += coeffs(i, 0) * dq;
            grad[1] += coeffs(i, 1) * dq;
            dxx += coeffs(i, 2) * dq;
            dyy += coeffs(i, 4) * dq;
            i++;
        }

        CHECK(5.0 == Approx(grad[0]));
        CHECK(2.0 == Approx(grad[1]));
        CHECK(10.0 == Approx(dxx));
        CHECK(4.0 == Approx(dyy));
    }
}

TEST_CASE("quadratic LSQ coefficient calculation - 3D") {
    auto unit_cube_points = buildUnitCubePoints(3, 3, 3);
    Parfait::Point<double> center = {0.5, 0.5, 0.5};

    auto get_stencil_point = [&](int n) { return unit_cube_points[n]; };
    int num_stencil_points = 27;
    double stencil_point_weight = 0.0;

    SECTION("Full QLSQ") {
        auto coeffs = Parfait::calcQLSQCoefficients(
            get_stencil_point, num_stencil_points, stencil_point_weight, center, Parfait::ThreeD);
        REQUIRE(num_stencil_points == coeffs.rows());
        REQUIRE(10 == coeffs.cols());

        int i = 0;
        double f = 0.0;
        Parfait::Point<double> grad{0, 0, 0};
        double dxx = 0;
        double dyy = 0;
        double dzz = 0;
        for (auto& p : unit_cube_points) {
            auto q = sphereTestFunction(p[0], p[1], p[2]);
            f += coeffs(i, 0) * q;
            grad[0] += coeffs(i, 1) * q;
            grad[1] += coeffs(i, 2) * q;
            grad[2] += coeffs(i, 3) * q;
            dxx += coeffs(i, 4) * q;
            dyy += coeffs(i, 7) * q;
            dzz += coeffs(i, 9) * q;
            i++;
        }

        auto center_value = sphereTestFunction(center[0], center[1], center[2]);
        CHECK(center_value == Approx(f));
        CHECK(5.0 == Approx(grad[0]));
        CHECK(2.0 == Approx(grad[1]));
        CHECK(-7.2 == Approx(grad[2]));
        CHECK(10.0 == Approx(dxx));
        CHECK(4.0 == Approx(dyy));
        CHECK(-14.4 == Approx(dzz));
    }
    SECTION("Only Gradient and Hessian QLSQ") {
        auto coeffs = Parfait::calcQLSQGradientHessianCoefficients(
            get_stencil_point, num_stencil_points, stencil_point_weight, center, Parfait::ThreeD);
        REQUIRE(num_stencil_points == coeffs.rows());
        REQUIRE(9 == coeffs.cols());

        auto center_value = sphereTestFunction(center[0], center[1], center[2]);

        int i = 0;
        double dxx = 0;
        double dyy = 0;
        double dzz = 0;
        Parfait::Point<double> grad{0, 0, 0};
        for (auto& p : unit_cube_points) {
            auto dq = sphereTestFunction(p[0], p[1], p[2]) - center_value;
            grad[0] += coeffs(i, 0) * dq;
            grad[1] += coeffs(i, 1) * dq;
            grad[2] += coeffs(i, 2) * dq;
            dxx += coeffs(i, 3) * dq;
            dyy += coeffs(i, 6) * dq;
            dzz += coeffs(i, 8) * dq;
            i++;
        }

        CHECK(5.0 == Approx(grad[0]));
        CHECK(2.0 == Approx(grad[1]));
        CHECK(-7.2 == Approx(grad[2]));
        CHECK(10.0 == Approx(dxx));
        CHECK(4.0 == Approx(dyy));
        CHECK(-14.4 == Approx(dzz));
    }
}

double interpolate(int num_points, const double* points, const double* solution, const double* query_point) {
    auto get_stencil_point = [&](int n) {
        return Parfait::Point<double>(points[3 * n + 0], points[3 * n + 1], points[3 * n + 2]);
    };
    int num_stencil_points = num_points;
    double stencil_point_weight = 0.0;
    Parfait::Point<double> q{query_point[0], query_point[1], query_point[2]};
    auto coeffs =
        Parfait::calcLLSQCoefficients(get_stencil_point, num_stencil_points, stencil_point_weight, q, Parfait::ThreeD);
    double interpolated_value = 0.0;
    for (int i = 0; i < num_stencil_points; i++) {
        interpolated_value += solution[i] * coeffs(i, 0);
    }
    return interpolated_value;
}
