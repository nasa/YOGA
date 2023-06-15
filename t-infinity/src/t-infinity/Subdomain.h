#pragma once
#include <utility>
#include <vector>
#include <parfait/VectorTools.h>
#include "CellIdFilter.h"

namespace inf {
namespace Subdomain {
    enum NODE_TYPE { OUTSIDE_DOMAIN = -1 };
    template <typename Sync>
    std::vector<int> labelNodeDistance(const std::set<int>& domain_nodes,
                                       int max_distance,
                                       const std::vector<std::vector<int>>& n2n,
                                       const Sync& sync) {
        std::vector<int> colors(n2n.size(), OUTSIDE_DOMAIN);
        for (int node_id : domain_nodes) {
            colors[node_id] = 0;
        }
        for (int i = 0; i < max_distance; ++i) {
            sync(colors);
            for (size_t node_id = 0; node_id < n2n.size(); ++node_id) {
                if (colors[node_id] == i) {
                    for (int n : n2n[node_id]) {
                        if (colors[n] == OUTSIDE_DOMAIN) colors[n] = i + 1;
                    }
                }
            }
        }
        sync(colors);
        return colors;
    }
    template <typename Sync>
    std::vector<int> buildNodeColors(const std::set<int>& domain_nodes,
                                     int overlap,
                                     const std::vector<std::vector<int>>& n2n,
                                     const Sync& sync) {
        return labelNodeDistance(domain_nodes, 2 * overlap, n2n, sync);
    }

    template <typename Sync>
    std::set<int> addNeighborNodes(std::set<int> domain_nodes,
                                   int overlap,
                                   const std::vector<std::vector<int>>& n2n,
                                   const Sync& sync) {
        auto node_distance = labelNodeDistance(domain_nodes, overlap, n2n, sync);
        for (int node_id = 0; node_id < (int)node_distance.size(); ++node_id) {
            if (node_distance[node_id] != OUTSIDE_DOMAIN) domain_nodes.insert(node_id);
        }
        return domain_nodes;
    }

    inline std::shared_ptr<CellIdFilter> buildSubdomainFilter(
        MPI_Comm comm,
        std::shared_ptr<MeshInterface> mesh,
        const std::vector<int>& subdomain_nodes) {
        std::vector<double> subdomain_node_field(mesh->nodeCount(), 0.0);
        for (int node_id : subdomain_nodes) subdomain_node_field[node_id] = 1.0;
        auto selector = std::make_shared<NodeValueSelector>(1.0, 1.0, subdomain_node_field);
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

}
}