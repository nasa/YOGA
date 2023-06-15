#pragma once
#include <parfait/Point.h>

namespace Parfait {

inline Parfait::Point<double> closestPointOnEdge(const Parfait::Point<double>& start,
                                                 const Parfait::Point<double>& end,
                                                 const Parfait::Point<double>& query) {
    auto edge_vector = end - start;
    auto point_vector = query - start;
    auto edge_length_squared = edge_vector.magnitudeSquared();
    edge_length_squared = std::max(edge_length_squared, 1e-200);  // protect against zero length edges.
    auto t = Parfait::Point<double>::dot(point_vector, edge_vector) / edge_length_squared;
    if (t < 0) {
        return start;
    } else if (t > 1) {
        return end;
    } else {
        return start + t * edge_vector;
    }
}
}
