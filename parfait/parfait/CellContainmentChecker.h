#pragma once

#include "Point.h"
#include "CellTesselator.h"
#include "Barycentric.h"
#include "ToString.h"
#include "CGNSElements.h"

namespace Parfait {
class CellContainmentChecker {
  public:
    static bool isInCell_c(double* vertices, int size, double* xyz) {
        Parfait::Point<double> p{xyz[0], xyz[1], xyz[2]};
        if (4 == size) {
            auto cell = CellContainmentChecker::convertToArrayOfPoints<4>(vertices);
            return CellContainmentChecker::isInCell<4>(cell, p);
        } else if (5 == size) {
            auto cell = CellContainmentChecker::convertToArrayOfPoints<5>(vertices);
            return CellContainmentChecker::isInCell<5>(cell, p);
        } else if (6 == size) {
            auto cell = CellContainmentChecker::convertToArrayOfPoints<6>(vertices);
            return CellContainmentChecker::isInCell<6>(cell, p);
        } else if (8 == size) {
            auto cell = CellContainmentChecker::convertToArrayOfPoints<8>(vertices);
            return CellContainmentChecker::isInCell<8>(cell, p);
        } else {
            throw std::domain_error("Invalid cell size: " + std::to_string(size));
        }
    }

    template <int N>
    static bool isInCell(const std::array<Parfait::Point<double>, N>& cell, const Parfait::Point<double>& p) {
        auto tets = CellTesselator::tesselate(cell);
        for (auto& tet : tets)
            if (isInTet(tet, p)) return true;
        return false;
    }

    template <int N>
    static std::array<Parfait::Point<double>, N> convertToArrayOfPoints(double* vertices) {
        std::array<Parfait::Point<double>, N> points;
        for (int i = 0; i < N; i++) {
            points[i] = {vertices[3 * i], vertices[3 * i + 1], vertices[3 * i + 2]};
        }
        return points;
    }

  private:
    template <typename Container>
    static bool isInTet(const Container& tet, const Parfait::Point<double>& p) {
        auto weights = calculateBarycentricCoordinates(tet, p);

        int worst = 0;
        for (size_t i = 1; i < weights.size(); i++)
            if (weights[i] < weights[worst]) worst = i;

        double longest = 0.0;
        double shortest = 1.0e10;
        for (auto& edge : Parfait::CGNS::Tet::edge_to_node) {
            double length = (tet[edge[0]] - tet[edge[1]]).magnitude();
            longest = std::max(longest, length);
            shortest = std::min(shortest, length);
        }
        double edge_ratio = longest / shortest;

        double tol = 1.0e-13 * edge_ratio;
        return weights[worst] + tol > 0.0;
    }
};
}