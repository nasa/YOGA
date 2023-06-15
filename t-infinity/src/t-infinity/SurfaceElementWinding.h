#pragma once
#include <memory>
#include <vector>
#include <parfait/Point.h>
#include <t-infinity/TinfMesh.h>

namespace SurfaceElementWinding {
bool areSurfaceElementsWoundOut(const inf::TinfMesh& mesh,
                                const std::vector<int>& surface_neighbors);

void windAllSurfaceElementsOut(MessagePasser mp,
                               inf::TinfMesh& mesh,
                               const std::vector<int>& surface_neighbors);

Parfait::Point<double> computeTriangleArea(const Parfait::Point<double>& a,
                                           const Parfait::Point<double>& b,
                                           const Parfait::Point<double>& c);

}
