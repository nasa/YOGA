#pragma once
#include <array>
#include "Point.h"

namespace Parfait {
namespace LagrangeTriangle {
    class P2 {
      public:
        P2(Parfait::Point<double> p1,
           Parfait::Point<double> p2,
           Parfait::Point<double> p3,
           Parfait::Point<double> p4,
           Parfait::Point<double> p5,
           Parfait::Point<double> p6);
        P2(const std::array<Parfait::Point<double>, 6>& p);
        Parfait::Point<double> evaluate(double r, double s) const;
        Parfait::Point<double> normal(double r, double s) const;
        Parfait::Point<double> closestNode(const Parfait::Point<double>& query) const;
        double lineSearchScaling(const Parfait::Point<double>& query, double dr, double ds, double r, double s) const;
        Parfait::Point<double> closest(const Parfait::Point<double>& query) const;
        std::vector<Parfait::Point<double>> sample(int num_points);

      public:
        std::array<Parfait::Point<double>, 6> points;

      private:
        Parfait::Point<double> calc_dBdr(double r, double s) const;
        Parfait::Point<double> calc_dBds(double r, double s) const;
        std::array<double, 2> calcResidual(const Parfait::Point<double>& query, double r, double s) const;
        Parfait::Point<double> calcRayFromSurfaceToPoint(const Parfait::Point<double>& query_point, double r, double s);
        std::array<double, 6> evaluateBasis(double r, double s) const;
        std::array<double, 6> evaluateBasisDr(double r, double s) const;
        std::array<double, 6> evaluateBasisDs(double r, double s) const;
    };
}
}

#include "LagrangeTriangle.hpp"
