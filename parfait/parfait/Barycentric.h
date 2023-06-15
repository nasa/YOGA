#pragma once
#include <array>
#include "Point.h"
#include "DenseMatrix.h"

namespace Parfait {
inline std::array<double, 3> calcBaryCentricWeights(const Parfait::Point<double>& a,
                                                    const Parfait::Point<double>& b,
                                                    const Parfait::Point<double>& c,
                                                    const Parfait::Point<double>& point_in_tri) {
    Parfait::Point<double> l = {0, 0, 0};
    Parfait::Point<double> v0 = b - a;
    Parfait::Point<double> v1 = c - a;
    Parfait::Point<double> v2 = point_in_tri - a;
    double denominator = (Parfait::Point<double>::dot(v0, v0) * Parfait::Point<double>::dot(v1, v1) -
                          Parfait::Point<double>::dot(v0, v1) * Parfait::Point<double>::dot(v1, v0));
    double numerator1 = (Parfait::Point<double>::dot(v1, v1) * Parfait::Point<double>::dot(v2, v0) -
                         Parfait::Point<double>::dot(v1, v0) * Parfait::Point<double>::dot(v2, v1));
    double numerator2 = (Parfait::Point<double>::dot(v0, v0) * Parfait::Point<double>::dot(v2, v1) -
                         Parfait::Point<double>::dot(v0, v1) * Parfait::Point<double>::dot(v2, v0));

    if (denominator != 0.0) {
        l[1] = numerator1 / denominator;
        l[2] = numerator2 / denominator;
        l[0] = 1.0 - l[1] - l[2];
    }
    return {l[0], l[1], l[2]};
}

namespace BarycentricTet {
    inline Parfait::Point<double> calcTetCentroid(const std::array<Parfait::Point<double>, 4>& tet) {
        Parfait::Point<double> c{0, 0, 0};
        for (auto& p : tet) c += p;
        return c * .25;
    }

    inline void shiftToOrigin(std::array<double, 4>& x,
                              std::array<double, 4>& y,
                              std::array<double, 4>& z,
                              const Parfait::Point<double>& centroid) {
        for (int i = 0; i < 4; i++) {
            x[i] -= centroid[0];
            y[i] -= centroid[1];
            z[i] -= centroid[2];
        }
    }
    inline double calcScalingFactor(const std::array<double, 4>& x,
                                    const std::array<double, 4>& y,
                                    const std::array<double, 4>& z) {
        double max_distance = 0.0;
        for (int i = 0; i < 4; i++) {
            Parfait::Point<double> p{x[i], y[i], z[i]};
            max_distance = std::max(max_distance, p.magnitude());
        }
        return 1.0 / max_distance;
    }
    inline void scale(std::array<double, 4>& x,
                      std::array<double, 4>& y,
                      std::array<double, 4>& z,
                      double scaling_factor) {
        for (int i = 0; i < 4; i++) {
            x[i] *= scaling_factor;
            y[i] *= scaling_factor;
            z[i] *= scaling_factor;
        }
    }
}

// for math, see:
// http://en.wikipedia.org/wiki/Barycentric_coordinate_system#Barycentric_coordinates_on_tetrahedra
inline std::array<double, 4> calculateBarycentricCoordinates(const std::array<Parfait::Point<double>, 4>& tet,
                                                             const Parfait::Point<double>& point) {
    std::array<double, 4> x = {tet[0][0], tet[1][0], tet[2][0], tet[3][0]};
    std::array<double, 4> y = {tet[0][1], tet[1][1], tet[2][1], tet[3][1]};
    std::array<double, 4> z = {tet[0][2], tet[1][2], tet[2][2], tet[3][2]};
    auto centroid = BarycentricTet::calcTetCentroid(tet);
    BarycentricTet::shiftToOrigin(x, y, z, centroid);
    auto scaling_factor = BarycentricTet::calcScalingFactor(x, y, z);
    BarycentricTet::scale(x, y, z, scaling_factor);

    using namespace Parfait;
    Parfait::Vector<double, 3> p{point[0] - centroid[0], point[1] - centroid[1], point[2] - centroid[2]};
    for (int i = 0; i < 3; i++) p[i] *= scaling_factor;

    Parfait::Vector<double, 3> v4{x[3], y[3], z[3]};

    Parfait::DenseMatrix<double, 3, 3> T{{x[0] - x[3], x[1] - x[3], x[2] - x[3]},
                                         {y[0] - y[3], y[1] - y[3], y[2] - y[3]},
                                         {z[0] - z[3], z[1] - z[3], z[2] - z[3]}};

    Parfait::Vector<double, 3> b = p - v4;

    auto lambdas = GaussJordan::solve(T, b);

    double sum = lambdas[0] + lambdas[1] + lambdas[2];
    return {lambdas[0], lambdas[1], lambdas[2], 1.0 - sum};
}
}