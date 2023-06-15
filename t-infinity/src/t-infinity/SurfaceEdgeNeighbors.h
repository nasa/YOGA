#pragma once
#include <parfait/StitchMesh.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/FaceNeighbors.h>
#include "VectorFieldAdapter.h"

namespace inf {
namespace SurfaceEdgeNeighbors {

    std::vector<std::vector<int>> buildSurfaceEdgeNeighbors(const inf::MeshInterface& mesh);
    int numberOfEdges(inf::MeshInterface::CellType type);

    bool edgeHasNode(const std::array<int, 2>& edge, int node);
    int getOtherNode(const std::array<int, 2>& edge, int node);

}

}
