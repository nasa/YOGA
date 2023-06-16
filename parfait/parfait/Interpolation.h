#pragma once
#include <array>
#include "Point.h"
#include "DenseMatrix.h"
#include "LeastSquaresReconstruction.h"

namespace Parfait {
std::array<double, 4> leastSquaresPlaneCoefficients(int n, const double* points, const double* solution);

inline double interpolate(int n, const double* points, const double* solutions, const double* query_point) {
    auto c = leastSquaresPlaneCoefficients(n, points, solutions);
    return query_point[0] * c[0] + query_point[1] * c[1] + query_point[2] * c[2] + c[3];
}

inline double interpolate_strict(int num_points,
                                 const double* points,
                                 const double* solution,
                                 const double* query_point) {
    auto interpolated_value = interpolate(num_points, points, solution, query_point);
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
    for (int i = 0; i < num_points; i++) {
        min = std::min(min, solution[i]);
        max = std::max(max, solution[i]);
    }
    if (interpolated_value < min) {
        interpolated_value = min;
    }
    if (interpolated_value > max) {
        interpolated_value = max;
    }
    return interpolated_value;
}
inline std::array<double, 4> leastSquaresPlaneCoefficients(int n, const double* points, const double* solution) {
    std::array<double, 4> coefficients;
    double sum_x = 0, sum_y = 0, sum_z = 0, sum_xy = 0, sum_xz = 0, sum_yz = 0, sum_xx = 0, sum_yy = 0, sum_zz = 0,
           sum_fx = 0, sum_fy = 0, sum_fz = 0, sum_f = 0;
    for (int i = 0; i < n; i++) {
        double x, y, z, f;
        auto p = &points[3 * i];
        x = p[0];
        y = p[1];
        z = p[2];
        f = solution[i];
        sum_x += x;
        sum_y += y;
        sum_z += z;
        sum_xy += x * y;
        sum_xz += x * z;
        sum_yz += y * z;
        sum_xx += x * x;
        sum_yy += y * y;
        sum_zz += z * z;
        sum_fx += f * x;
        sum_fy += f * y;
        sum_fz += f * z;
        sum_f += f;
    }

    Parfait::DenseMatrix<double, 4, 4> A{{sum_xx, sum_xy, sum_xz, sum_x},
                                         {sum_xy, sum_yy, sum_yz, sum_y},
                                         {sum_xz, sum_yz, sum_zz, sum_z},
                                         {sum_x, sum_y, sum_z, double(n)}};

    Parfait::Vector<double, 4> b{sum_fx, sum_fy, sum_fz, sum_f};

    auto x = Parfait::GaussJordan::solve(A, b);
    coefficients = {x[0], x[1], x[2], x[3]};
    return coefficients;
}

}