#include "Hiro.h"
#include "Cell.h"

namespace inf {

Parfait::Point<double> Hiro::calcCentroidFor2dElement(
    const std::vector<Parfait::Point<double>>& points, double p) {
    double max_area = 0;
    double total_area = 0;
    Parfait::Point<double> centroid = {0, 0, 0};
    for (size_t i = 0; i < points.size(); i++) {
        auto& p1 = points[i];
        auto& p2 = points[(i + 1) % points.size()];
        double area = Parfait::Point<double>::distance(p1, p2);
        max_area = std::max(area, max_area);
    }
    for (size_t i = 0; i < points.size(); i++) {
        auto& p1 = points[i];
        auto& p2 = points[(i + 1) % points.size()];
        double area = Parfait::Point<double>::distance(p1, p2);
        double normalized_area = area / max_area;
        double raised_area = std::pow(normalized_area, p);
        auto edge_midpoint = 0.5 * (p1 + p2);
        centroid += edge_midpoint * raised_area;
        total_area += raised_area;
    }
    centroid /= total_area;
    return centroid;
}

Parfait::Point<double> Hiro::calcCentroid(const inf::Cell& cell, double p) {
    if (inf::MeshInterface::TRI_3 == cell.type() or inf::MeshInterface::QUAD_4 == cell.type()) {
        return calcCentroidFor2dElement(cell.points(), p);
    }
    double max_area = 0;
    double total_area = 0;
    Parfait::Point<double> centroid = {0, 0, 0};
    for (int f = 0; f < cell.faceCount(); f++) {
        double area = cell.faceAreaNormal(f).magnitude();
        max_area = std::max(area, max_area);
    }
    for (int f = 0; f < cell.faceCount(); f++) {
        double area = cell.faceAreaNormal(f).magnitude();
        double normalized_area = area / max_area;
        double raised_area = std::pow(normalized_area, p);
        auto face_midpoint = calcCentroidFor2dElement(cell.facePoints(f), p);
        centroid += face_midpoint * raised_area;
        total_area += raised_area;
    }
    centroid /= total_area;
    return centroid;
}
}
