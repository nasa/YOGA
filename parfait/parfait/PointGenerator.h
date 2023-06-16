#pragma once
#include <chrono>
#include <random>
#include <vector>
#include "Extent.h"
#include "Point.h"
#include "ExtentBuilder.h"

namespace Parfait {
inline std::vector<Parfait::Point<double>> generateRandomPoints(int num_points,
                                                                Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}},
                                                                int seed = 42) {
    std::mt19937_64 rng;
    rng.seed(seed);
    std::uniform_real_distribution<double> random_x(e.lo[0], e.hi[0]);
    std::uniform_real_distribution<double> random_y(e.lo[1], e.hi[1]);
    std::uniform_real_distribution<double> random_z(e.lo[2], e.hi[2]);
    std::vector<Parfait::Point<double>> points(num_points);
    for (int i = 0; i < num_points; i++) {
        points[i][0] = random_x(rng);
        points[i][1] = random_y(rng);
        points[i][2] = random_z(rng);
    }
    return points;
}

inline std::vector<Parfait::Point<double>> generateRandomPointsOnCircleInXYPlane(int num_points,
                                                                                 Parfait::Point<double> center,
                                                                                 double radius,
                                                                                 int seed = 42) {
    std::mt19937_64 rng;
    rng.seed(seed);
    std::uniform_real_distribution<double> random_theta(0.0, 360.0);
    std::vector<Parfait::Point<double>> points(num_points);
    for (int i = 0; i < num_points; i++) {
        double angle = random_theta(rng) * M_PI / 180.0;
        auto& p = points[i];
        p[0] = radius * cos(angle);
        p[1] = radius * sin(angle);
        p[2] = 0.0;
        p += center;
    }
    return points;
}

inline std::vector<Parfait::Point<double>> generateOrderedPointsOnCircleInXYPlane(int num_points,
                                                                                  Parfait::Point<double> center,
                                                                                  double radius) {
    std::vector<Parfait::Point<double>> points(num_points);
    double d_angle = 360 / double(num_points - 1);
    for (int i = 0; i < num_points; i++) {
        double angle = d_angle * i * M_PI / 180.0;
        auto& p = points[i];
        p[0] = radius * cos(angle);
        p[1] = radius * sin(angle);
        p[2] = 0.0;
        p += center;
    }
    return points;
}

inline std::vector<Parfait::Point<double>> generateCartesianPoints(int num_x,
                                                                   int num_y,
                                                                   int num_z,
                                                                   Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}}) {
    std::vector<Parfait::Point<double>> points;
    double dx = (e.hi[0] - e.lo[0]) / double(num_x - 1);
    double dy = (e.hi[1] - e.lo[1]) / double(num_y - 1);
    double dz = (e.hi[2] - e.lo[2]) / double(num_z - 1);
    for (int i = 0; i < num_x; i++) {
        for (int j = 0; j < num_y; j++) {
            for (int k = 0; k < num_z; k++) {
                Parfait::Point<double> p;
                p[0] = e.lo[0] + i * dx;
                p[1] = e.lo[1] + j * dy;
                p[2] = e.lo[2] + k * dz;
                points.push_back(p);
            }
        }
    }

    return points;
}

template <size_t N = 3>
void wiggle(std::vector<Parfait::Point<double, N>>& points, double wiggle_distance) {
    std::mt19937_64 rng;
    rng.seed(42);
    std::uniform_real_distribution<double> random(-1.0, 1.0);
    for (auto& p : points) {
        Parfait::Point<double, N> w;
        for (size_t i = 0; i < N; i++) {
            w[i] = random(rng);
        }
        w.normalize();
        w *= wiggle_distance;
        p += w;
    }
}

template <size_t N = 3>
void wiggle(std::vector<Parfait::Point<double, N>>& points) {
    if (points.size() == 0) return;
    auto domain = Parfait::ExtentBuilder::build<decltype(points), N>(points);
    int dim = domain.longestDimension();
    double range = domain.hi[dim] - domain.lo[dim];
    double wiggle_distance = 1.0e-8 * range;
    wiggle(points, wiggle_distance);
}

}
