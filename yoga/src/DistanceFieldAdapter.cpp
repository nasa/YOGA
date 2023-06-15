#include <Tracer.h>
#include "DistanceFieldAdapter.h"
#include "NanoFlannDistanceCalculator.h"
#include "ParallelSurface.h"

namespace YOGA {

std::vector<double> DistanceFieldAdapter::getNodeDistances(
    const YogaMesh& m, const std::vector<std::vector<Parfait::Point<double>>>& surfaces) {
    Tracer::begin("build nanoflann tree");
    NanoFlannDistanceCalculator distance_calculator(surfaces);
    Tracer::end("build nanoflann tree");
    Tracer::begin("get distances");
    auto query_points = extractQueryPoints(m);
    auto grid_ids_for_nodes = extractGridIds(m);
    auto d2 = distance_calculator.calculateDistances(query_points, grid_ids_for_nodes);
    Tracer::end("get distances");
    return d2;
}

std::vector<Parfait::Point<double>> DistanceFieldAdapter::extractQueryPoints(const YogaMesh& mesh) {
    std::vector<Parfait::Point<double>> points;
    for (int i = 0; i < mesh.nodeCount(); i++) points.push_back(mesh.getNode<double>(i));
    return points;
}

std::vector<int> DistanceFieldAdapter::extractGridIds(const YogaMesh& mesh) {
    std::vector<int> grid_ids(mesh.nodeCount());
    for (int i = 0; i < mesh.nodeCount(); i++) {
        grid_ids[i] = mesh.getAssociatedComponentId(i);
    }
    return grid_ids;
}

}
