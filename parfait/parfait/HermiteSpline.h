#pragma once
#include <parfait/Point.h>
namespace Parfait {
class HermiteSpline {
  public:
    HermiteSpline(const Parfait::Point<double>& p0,
                  const Parfait::Point<double>& p1,
                  const Parfait::Point<double>& grad0,
                  const Parfait::Point<double>& grad1)
        : p0(p0), p1(p1), grad_a(grad0), grad_b(grad1), n(1000), total_length(integrate(0, 1)) {}

    Parfait::Point<double> evaluate(double u) const {
        auto u3 = u * u * u;
        auto u2 = u * u;
        return (2 * u3 - 3 * u2 + 1) * p0 + (u3 - 2 * u2 + u) * grad_a + (-2 * u3 + 3 * u2) * p1 + (u3 - u2) * grad_b;
    }

    double integrate(double u0, double u1) const {
        double length = 0.0;
        double du = (u1 - u0) / double(n);
        for (int i = 0; i < n; i++) {
            double a = u0 + i * du;
            double b = u0 + (i + 1) * du;
            length += calcSegmentLength(a, b);
        }
        return length;
    }

    double calcUAtFractionOfLength(double frac) const {
        double target_length = frac * total_length;
        double integrated_length = 0.0;
        double du = 1.0 / double(n);
        double u = 0.0;
        for (int i = 0; i <= n; i++) {
            u = du * i;
            double left = integrated_length;
            integrated_length += calcSegmentLength(u, u + du);
            double right = integrated_length;
            if (left <= target_length and right >= target_length) {
                double alpha = (target_length - left) / (right - left);
                u = (1 - alpha) * u + alpha * (u + du);
                break;
            }
        }
        return u;
    }

  private:
    const Parfait::Point<double> p0, p1, grad_a, grad_b;
    const int n = 1000;
    const double total_length;

    double calcSegmentLength(double a, double b) const { return (evaluate(b) - evaluate(a)).magnitude(); }
};
}