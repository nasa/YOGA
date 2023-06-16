#pragma once
#include <functional>
#include <parfait/Point.h>
#include "MeshInterface.h"
#include "Cell.h"

namespace inf {
namespace MeshQuality {

    std::vector<double> cellHilbertCost(const inf::MeshInterface& mesh);
    std::vector<double> cellFlattnessCost(const inf::MeshInterface& mesh);
    std::vector<double> nodeHilbertCost(const inf::MeshInterface& mesh,
                                        const std::vector<std::vector<int>>& n2c);
    double cellHilbertCost(inf::MeshInterface::CellType type,
                           const std::vector<Parfait::Point<double>>& points);
    double cellHilbertCost(const inf::MeshInterface& mesh, int c);
    double nodeHilbertCost(const inf::MeshInterface& mesh,
                           const std::vector<std::vector<int>>& n2c,
                           int n);
    double nodeCost(const inf::MeshInterface& mesh,
                    const std::vector<std::vector<int>>& n2c,
                    int n,
                    std::function<double(const MeshInterface&, int)> cell_cost_function);
    double nodeMaxHilbertCost(const inf::MeshInterface& mesh,
                              const std::vector<std::vector<int>>& n2c,
                              int n);
    std::vector<double> cellConditionNumber(const inf::MeshInterface& mesh);

    namespace Flattness {
        inline double quad(const Parfait::Point<double>& p0,
                           const Parfait::Point<double>& p1,
                           const Parfait::Point<double>& p2,
                           const Parfait::Point<double>& p3) {
            auto v1 = p1 - p0;
            auto v2 = p2 - p0;
            auto v3 = p3 - p0;

            auto a = Parfait::Point<double>::cross(v1, v2);
            auto b = Parfait::Point<double>::cross(v2, v3);
            a.normalize();
            b.normalize();
            return 1.0 - Parfait::Point<double>::dot(a, b);
        }

        inline double cell(const MeshInterface& mesh, int c) {
            inf::Cell cell(mesh, c);
            double cost = 0.0;
            int count = 0;
            for (int f = 0; f < cell.faceCount(); f++) {
                auto face_points = cell.facePoints(f);
                if (face_points.size() == 4) {
                    cost += quad(face_points[0], face_points[1], face_points[2], face_points[3]);
                    count++;
                }
            }

            if (count == 0) return 0.0;
            return cost / double(count);
        }
    }

    namespace CornerConditionNumber {
        double quad(Parfait::Point<double> e1, Parfait::Point<double> e2);
        double hex(Parfait::Point<double> e1, Parfait::Point<double> e2, Parfait::Point<double> e3);
        double isotropicTet(Parfait::Point<double> e1,
                            Parfait::Point<double> e2,
                            Parfait::Point<double> e3);

        double prism(Parfait::Point<double> e1,
                     Parfait::Point<double> e2,
                     Parfait::Point<double> e3);
        double pyramidBottomCorner(Parfait::Point<double> e1,
                                   Parfait::Point<double> e2,
                                   Parfait::Point<double> e3);
        double pyramidTopCorner(Parfait::Point<double> e1,
                                Parfait::Point<double> e2,
                                Parfait::Point<double> e3);
    }
    namespace ConditionNumber {
        double tri(Parfait::Point<double> p0, Parfait::Point<double> p1, Parfait::Point<double> p2);
        double quad(Parfait::Point<double> p0,
                    Parfait::Point<double> p1,
                    Parfait::Point<double> p2,
                    Parfait::Point<double> p3);
        double tet(Parfait::Point<double> p0,
                   Parfait::Point<double> p1,
                   Parfait::Point<double> p2,
                   Parfait::Point<double> p3);

        double hex(Parfait::Point<double> p0,
                   Parfait::Point<double> p1,
                   Parfait::Point<double> p2,
                   Parfait::Point<double> p3,
                   Parfait::Point<double> p4,
                   Parfait::Point<double> p5,
                   Parfait::Point<double> p6,
                   Parfait::Point<double> p7);

        double prism(Parfait::Point<double> p0,
                     Parfait::Point<double> p1,
                     Parfait::Point<double> p2,
                     Parfait::Point<double> p3,
                     Parfait::Point<double> p4,
                     Parfait::Point<double> p5);

        double pyramid(Parfait::Point<double> p0,
                       Parfait::Point<double> p1,
                       Parfait::Point<double> p2,
                       Parfait::Point<double> p3,
                       Parfait::Point<double> p4);
    }
}
}
