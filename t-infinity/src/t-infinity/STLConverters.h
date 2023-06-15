#pragma once
#include <vector>
#include <parfait/Point.h>
#include <parfait/StitchMesh.h>
#include "t-infinity/MeshInterface.h"
#include "TinfMesh.h"
#include "StitchMesh.h"

namespace inf {
namespace STLConverters {
    inline std::shared_ptr<inf::TinfMesh> stitchSTLToInfinity(
        const std::vector<Parfait::Facet>& facets) {
        std::vector<int> cell_nodes;
        std::vector<Parfait::Point<double>> points;
        std::tie(cell_nodes, points) = Parfait::stitchFacets(facets);
        int num_tris = cell_nodes.size() / 3;
        long num_nodes = points.size();

        inf::TinfMeshData mesh_data;
        mesh_data.cells[inf::MeshInterface::TRI_3] = cell_nodes;
        mesh_data.global_cell_id[inf::MeshInterface::TRI_3] = std::vector<long>();
        mesh_data.cell_tags[inf::MeshInterface::TRI_3] = std::vector<int>();
        mesh_data.cell_owner[inf::MeshInterface::TRI_3] = std::vector<int>();
        for (long i = 0; i < num_tris; i++) {
            mesh_data.global_cell_id[inf::MeshInterface::TRI_3].push_back(i);
            mesh_data.cell_owner[inf::MeshInterface::TRI_3].push_back(0);
            mesh_data.cell_tags[inf::MeshInterface::TRI_3].push_back(facets[i].tag);
        }

        for (long i = 0; i < num_nodes; i++) {
            mesh_data.global_node_id.push_back(i);
            mesh_data.node_owner.push_back(0);
        }
        mesh_data.points = points;

        auto mesh = std::make_shared<inf::TinfMesh>(mesh_data, 0);
        return StitchMesh::stitchMeshes({mesh});
    }

    inline std::vector<Parfait::Point<double>> extractPoints(const inf::MeshInterface& mesh) {
        std::vector<Parfait::Point<double>> points(mesh.nodeCount());
        for (unsigned long i = 0; i < points.size(); i++) {
            points[i] = mesh.node(i);
        }
        return points;
    }
}
}
