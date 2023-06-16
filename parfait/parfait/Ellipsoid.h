#pragma once
#include <parfait/Point.h>
#include <parfait/MotionMatrix.h>

namespace Parfait {

template <typename T>
class Ellipsoid {
  public:
    using P = const Point<T>&;
    Ellipsoid(P centroid, double radius_a, double radius_b, double radius_c)
        : centroid(centroid), a(radius_a), b(radius_b), c(radius_c) {}

    Ellipsoid(P centroid, double radius_a, double radius_b, double radius_c, double theta, double phi)
        : centroid(centroid), a(radius_a), b(radius_b), c(radius_c), theta(theta), phi(phi) {}

    bool contains(P point) {
        Parfait::Point<T> shifted = point - centroid;
        Parfait::Point<T> origin{0, 0, 0}, z{0, 0, 1}, y{0, 1, 0};
        MotionMatrix m;
        m.addRotation(origin.data(), y.data(), -phi);
        m.addRotation(origin.data(), z.data(), -theta);
        m.movePoint(shifted.data());
        return square(shifted[0] / a) + square(shifted[1] / b) + square(shifted[2] / c) <= 1.0;
    }

  private:
    Parfait::Point<T> centroid;
    double a, b, c, theta = 0, phi = 0;

    double square(double d) { return d * d; }
    MotionMatrix transpose(const MotionMatrix& rotation) {
        std::array<double, 16> mat;
        rotation.getMatrix(mat.data());
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < i; j++) std::swap(mat[4 * i + j], mat[4 * j + i]);
        MotionMatrix m;
        m.setMotionMatrix(mat.data());
        return m;
    }
};
}
