#pragma once
#include <limits>
#include "LinearAlgebra.h"

namespace Parfait {
namespace LagrangeTriangle {

    inline P2::P2(
        Point<double> p1, Point<double> p2, Point<double> p3, Point<double> p4, Point<double> p5, Point<double> p6)
        : points{p1, p2, p3, p4, p5, p6} {}
    inline P2::P2(const std::array<Point<double>, 6>& p) : points(p) {}
    inline Point<double> P2::evaluate(double r, double s) const {
        auto basis = evaluateBasis(r, s);
        Point<double> p = {0.0, 0.0, 0.0};
        for (int i = 0; i < 6; i++) {
            p += basis[i] * points[i];
        }
        return p;
    }
    inline Point<double> P2::normal(double r, double s) const {
        auto dBds_vector = calc_dBds(r, s);
        auto dBdr_vector = calc_dBdr(r, s);
        auto n = Point<double>::cross(dBdr_vector, dBds_vector);
        n.normalize();
        return n;
    }
    inline Point<double> P2::closestNode(const Point<double>& query) const {
        double dist = std::numeric_limits<double>::max();
        Point<double> p = query;
        for (int i = 0; i < 6; i++) {
            auto d = Point<double>::distance(query, points[i]);
            if (d < dist) {
                dist = d;
                p = points[i];
            }
        }
        return p;
    }
    inline double P2::lineSearchScaling(const Point<double>& query, double dr, double ds, double r, double s) const {
        auto residual = calcResidual(query, r, s);
        double norm = std::sqrt(residual[0] * residual[0] + residual[1] * residual[1]);
        double scaling = 0.0;
        for (double l : {1.0, 0.5, 0.1}) {
            auto new_residual = calcResidual(query, r - l * dr, s - l * ds);
            double new_norm = std::sqrt(new_residual[0] * new_residual[0] + new_residual[1] * new_residual[1]);
            if (new_norm < norm) {
                norm = new_norm;
                scaling = l;
            }
        }
        return scaling;
    }
    inline Point<double> P2::closest(const Point<double>& query) const {
        double r = 0.25;
        double s = 0.25;
        double eps = 1.0e-6;
        auto residual = calcResidual(query, r, s);
        double norm = std::sqrt(residual[0] * residual[0] + residual[1] * residual[1]);
        for (int iter = 0; iter < 100; iter++) {
            if (norm < 1.0e-8) break;
            auto res_rp = calcResidual(query, r + eps, s);
            auto res_sp = calcResidual(query, r, s + eps);
            std::array<double, 4> A{};
            A[0] = (res_rp[0] - residual[0]) / eps;
            A[1] = (res_sp[0] - residual[0]) / eps;
            A[2] = (res_rp[1] - residual[1]) / eps;
            A[3] = (res_sp[1] - residual[1]) / eps;

            auto x = LinearAlgebra::solve(A, residual);
            if (fabs(x[0]) < 1.0e-10 and fabs(x[1]) < 1.0e-10) break;
            auto lambda = lineSearchScaling(query, x[0], x[1], r, s);
            r -= lambda * x[0];
            s -= lambda * x[1];

            if (r < 0) r = 0;
            if (s < 0) s = 0;
            if (r > 1.0) r = 1.0;
            if (s > 1.0) s = 1.0;
            if (r + s > 1.0) {
                double mag = std::sqrt(r * r + s * s);
                r /= mag;
                s /= mag;
            }
            residual = calcResidual(query, r, s);
            norm = std::sqrt(residual[0] * residual[0] + residual[1] * residual[1]);
        }

        return evaluate(r, s);
    }
    inline std::vector<Point<double>> P2::sample(int num_points) {
        std::vector<Point<double>> out;
        double du = 1.0 / double(num_points - 1.0);
        for (int i = 0; i < num_points; i++) {
            for (int j = 0; j < num_points; j++) {
                double r = du * i;
                double s = du * j;
                if (r + s <= 1.0) {
                    out.push_back(evaluate(r, s));
                }
            }
        }
        return out;
    }
    inline Point<double> P2::calc_dBdr(double r, double s) const {
        auto dBdr = evaluateBasisDr(r, s);
        Point<double> dBdr_vector = {0.0, 0.0, 0.0};
        for (int i = 0; i < 6; i++) dBdr_vector += dBdr[i] * points[i];
        return dBdr_vector;
    }
    inline Point<double> P2::calc_dBds(double r, double s) const {
        auto dBds = evaluateBasisDs(r, s);
        Point<double> dBds_vector = {0.0, 0.0, 0.0};
        for (int i = 0; i < 6; i++) dBds_vector += dBds[i] * points[i];
        return dBds_vector;
    }
    inline std::array<double, 2> P2::calcResidual(const Point<double>& query, double r, double s) const {
        auto p = evaluate(r, s);
        auto dBdr = calc_dBdr(r, s);
        auto dBds = calc_dBds(r, s);
        std::array<double, 2> res = {0.0, 0.0};
        auto v = query - p;
        res[0] = Point<double>::dot(v, dBdr);
        res[1] = Point<double>::dot(v, dBds);
        return res;
    }
    inline Point<double> P2::calcRayFromSurfaceToPoint(const Point<double>& query_point, double r, double s) {
        auto p = evaluate(r, s);
        auto v = query_point - p;
        v.normalize();

        return v;
    }
    inline std::array<double, 6> P2::evaluateBasis(double r, double s) const {
        return {1.0 - 3.0 * (r + s) + 2.0 * (r * r + s * s) + 4 * r * s,
                2.0 * r * (r - 0.5),
                2.0 * s * (s - 0.5),
                4.0 * r * (1.0 - r - s),
                4.0 * r * s,
                4.0 * s * (1.0 - r - s)};
    }
    inline std::array<double, 6> P2::evaluateBasisDr(double r, double s) const {
        return {-3.0 + 4.0 * r + 4.0 * s, 4.0 * r - 1.0, 0.0, 4.0 - 8.0 * r - 4.0 * s, 4.0 * s, -4.0 * s};
    }
    inline std::array<double, 6> P2::evaluateBasisDs(double r, double s) const {
        return {-3.0 + 4.0 * r + 4.0 * s, 0.0, 4.0 * s - 1.0, -4.0 * r, 4.0 * r, 4.0 - 4.0 * r - 8.0 * s};
    }
}
}
