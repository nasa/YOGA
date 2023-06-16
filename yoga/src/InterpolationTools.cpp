#include "YogaTypeSupport.h"
#include "InterpolationTools.h"
#include "InterpolationTools.hpp"
#include <parfait/DenseMatrix.h>

namespace YOGA {
void BarycentricInterpolation::calcWeightsTet(const double* tet, const double* p, double* weights) {
    std::array<Parfait::Point<double>, 4> vertices;
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = {tet[3 * i], tet[3 * i + 1], tet[3 * i + 2]};
    }
    auto w = calculateBarycentricCoordinates(vertices, p);
    std::copy(w.begin(),w.end(),weights);
}

std::array<double, 4> least_squares_plane_coefficients(int n, const double* points, const double* solution) {
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

double least_squares_interpolate(int n, const double* points, const double* solutions, const double* query_point) {
    auto c = least_squares_plane_coefficients(n, points, solutions);
    return query_point[0] * c[0] + query_point[1] * c[1] + query_point[2] * c[2] + c[3];
}

Parfait::Point<double> calcTetCentroid(const std::array<Parfait::Point<double>, 4>& tet) {
    Parfait::Point<double> c{0, 0, 0};
    for (auto& p : tet) c += p;
    return c * .25;
}

void shiftToOrigin(std::array<double, 4>& x,
                   std::array<double, 4>& y,
                   std::array<double, 4>& z,
                   const Parfait::Point<double>& centroid) {
    for (int i = 0; i < 4; i++) {
        x[i] -= centroid[0];
        y[i] -= centroid[1];
        z[i] -= centroid[2];
    }
}

double calcScalingFactor(const std::array<double, 4>& x,
                         const std::array<double, 4>& y,
                         const std::array<double, 4>& z) {
    double max_distance = 0.0;
    for (int i = 0; i < 4; i++) {
        Parfait::Point<double> p{x[i], y[i], z[i]};
        max_distance = std::max(max_distance, p.magnitude());
    }
    return 1.0 / max_distance;
}

void scale(std::array<double, 4>& x, std::array<double, 4>& y, std::array<double, 4>& z, double scaling_factor) {
    for (int i = 0; i < 4; i++) {
        x[i] *= scaling_factor;
        y[i] *= scaling_factor;
        z[i] *= scaling_factor;
    }
}
// for math, see:
// http://en.wikipedia.org/wiki/Barycentric_coordinate_system#Barycentric_coordinates_on_tetrahedra
std::array<double, 4> BarycentricInterpolation::calculateBarycentricCoordinates(const std::array<Parfait::Point<double>, 4>& tet,
                                                      const Parfait::Point<double>& point) {
    std::array<double, 4> x = {tet[0][0], tet[1][0], tet[2][0], tet[3][0]};
    std::array<double, 4> y = {tet[0][1], tet[1][1], tet[2][1], tet[3][1]};
    std::array<double, 4> z = {tet[0][2], tet[1][2], tet[2][2], tet[3][2]};
    auto centroid = calcTetCentroid(tet);
    shiftToOrigin(x, y, z, centroid);
    auto scaling_factor = calcScalingFactor(x, y, z);
    scale(x, y, z, scaling_factor);

    using namespace Parfait;
    Vector<double, 3> p{point[0] - centroid[0], point[1] - centroid[1], point[2] - centroid[2]};
    for (int i = 0; i < 3; i++) p[i] *= scaling_factor;

    Vector<double, 3> v4{x[3], y[3], z[3]};

    DenseMatrix<double, 3, 3> T{{x[0] - x[3], x[1] - x[3], x[2] - x[3]},
                                {y[0] - y[3], y[1] - y[3], y[2] - y[3]},
                                {z[0] - z[3], z[1] - z[3], z[2] - z[3]}};

    auto lambdas = GaussJordan::solve(T, Vector<double, 3>(p - v4));

    double sum = lambdas[0] + lambdas[1] + lambdas[2];
    return {lambdas[0], lambdas[1], lambdas[2], 1.0 - sum};
}
}

template class YOGA::FiniteElementInterpolation<double>;
template class YOGA::FiniteElementInterpolation<std::complex<double>>;

template class YOGA::LeastSquaresInterpolation<double>;
template class YOGA::LeastSquaresInterpolation<std::complex<double>>;
