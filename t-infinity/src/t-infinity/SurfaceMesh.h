#pragma once
#include <vector>
#include <array>
#include <memory>
#include "MeshInterface.h"
#include "TinfMesh.h"

namespace inf {
namespace SurfaceMesh {

    std::vector<std::array<int, 2>> selectExposedEdges(
        const inf::MeshInterface& mesh, const std::vector<std::vector<int>>& edge_neighbors);
    std::vector<std::vector<int>> identifyLoops(std::vector<std::array<int, 2>> edges);

    std::shared_ptr<inf::TinfMesh> fillHoles(const inf::MeshInterface& mesh_in);
    std::shared_ptr<inf::TinfMesh> fillHoles(const inf::MeshInterface& mesh_in,
                                             const std::vector<std::vector<int>>& loops);

    bool isManifold(const inf::MeshInterface& mesh,
                    const std::vector<std::vector<int>>& surface_edge_neighbors,
                    std::vector<int>* f = nullptr);

    bool isOriented(const inf::MeshInterface& mesh,
                    const std::vector<std::vector<int>>& surface_edge_neighbors);

    void taubinSmooth(inf::TinfMesh& mesh,
                      const std::vector<std::vector<int>>& surface_node_neighbors,
                      bool plot = false);
    std::shared_ptr<inf::TinfMesh> triangulate(const inf::MeshInterface& mesh);
    std::vector<double> aspectRatio(const inf::MeshInterface& mesh);
}
}