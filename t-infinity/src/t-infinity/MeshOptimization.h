#pragma once
#include "Cell.h"
#include "TinfMesh.h"
#include "MeshConnectivity.h"
#include "MeshHelpers.h"
#include "GlobalToLocal.h"
#include "MeshQualityMetrics.h"
#include "Extract.h"
#include <parfait/ToString.h>
#include <parfait/Throw.h>
#include <parfait/SyncField.h>
#include <parfait/DistanceTree.h>

namespace inf {

class MeshOptimization {
  public:
    enum NodeType { UNASSIGNED, FROZEN, EDGE, SURFACE, INTERIOR };
    MeshOptimization(MessagePasser mp, TinfMesh* mesh_in)
        : mp(mp),
          mesh(mesh_in),
          sync_pattern(buildNodeSyncPattern(mp, *mesh)),
          node_g2l(buildGlobalToLocalNode(mesh->mesh)) {
        n2n = inf::NodeToNode::build(*mesh);
        n2c_volume = inf::NodeToCell::buildVolumeOnly(*mesh);
        n2c_surface = inf::NodeToCell::buildSurfaceOnly(*mesh);
        frozen_tags = extractAllTagsWithDimensionality(mp, *mesh, 2);
        node_mark.resize(mesh->nodeCount(), 1);
        node_step_length = calcNodalStepLength();
        node_types.resize(mesh->nodeCount(), UNASSIGNED);
        node_projection_surface_tag.resize(mesh->nodeCount());
    }

    void setThreshold(double t) { hilbert_cost_threshold = t; }

    void setBeta(double b) {
        beta = b;
        node_step_length = calcNodalStepLength();
    }

    void enableWarp() { optimize_warp = true; }

    void disableWarp() { optimize_warp = false; }

    void setOmega(double w) { omega = w; }

    void setAlgorithm(std::string a) { algorithm = a; }

    bool areAnyTagsFrozen(const std::set<int>& tags) const {
        for (auto t : tags) {
            if (frozen_tags.count(t)) {
                return true;
            }
        }
        return false;
    }

    std::pair<NodeType, int> determineNodeType(const std::set<int>& adjacent_surface_tags) const {
        if (adjacent_surface_tags.size() == 0) {
            return {NodeType::INTERIOR, -1};
        }
        if (areAnyTagsFrozen(adjacent_surface_tags)) {
            return {NodeType::FROZEN, -1};
        }
        if (adjacent_surface_tags.size() == 1) {
            return {NodeType::SURFACE, *adjacent_surface_tags.begin()};
        }
        if (adjacent_surface_tags.size() > 1) {
            return {NodeType::FROZEN, -1};
        } else {
            PARFAIT_THROW("Could not determine node type.  Connected to tags " +
                          Parfait::to_string(adjacent_surface_tags));
        }
    }

    void setNodeTypes() {
        for (int n = 0; n < mesh->nodeCount(); n++) {
            std::set<int> surface_tags;
            for (auto c : n2c_surface[n]) {
                surface_tags.insert(mesh->cellTag(c));
            }
            auto orig_type = node_types[n];
            if (orig_type == UNASSIGNED) {
                NodeType type;
                int tag;
                std::tie(type, tag) = determineNodeType(surface_tags);
                node_types[n] = type;
                if (type == NodeType::SURFACE) {
                    node_projection_surface_tag[n] = tag;
                }
            }
        }
    }

    std::vector<double> getNodeTypes() const {
        std::vector<double> types(mesh->nodeCount());
        for (int n = 0; n < mesh->nodeCount(); n++) {
            types[n] = double(node_types[n]);
        }

        return types;
    }

    void setSlipTags(std::set<int> slip_tags_i) {
        slip_tags = slip_tags_i;
        for (auto t : slip_tags) {
            frozen_tags.erase(t);
        }
    }

    void finalize() {
        setNodeTypes();
        buildProjectionSurfaces();
    }

    Parfait::Point<double> nodeStepDirection(int n) {
        auto p = mesh->mesh.points[n];
        double eps = 1.0e-9;
        Parfait::Point<double> dx = {eps, 0, 0};
        Parfait::Point<double> dy = {0, eps, 0};
        Parfait::Point<double> dz = {0, 0, eps};

        auto cost = calcNodeCost(n);

        mesh->mesh.points[n] = p + dx;
        auto dcdx = (calcNodeCost(n) - cost) / eps;

        mesh->mesh.points[n] = p + dy;
        auto dcdy = (calcNodeCost(n) - cost) / eps;

        mesh->mesh.points[n] = p + dz;
        auto dcdz = (calcNodeCost(n) - cost) / eps;

        mesh->mesh.points[n] = p;

        p = {dcdx, dcdy, dcdz};
        p *= -1.0;
        p.normalize();
        return p;
    }

    double calcNodeCost(int n) {
        double cost =
            MeshQuality::nodeCost(*mesh, n2c_volume, n, [](const inf::MeshInterface& m, int cell) {
                return MeshQuality::cellHilbertCost(m, cell);
            });

        if (optimize_warp and cost < 0.7) {
            auto warp_cost_function = [](const inf::MeshInterface& m, int cell) {
                return MeshQuality::Flattness::cell(m, cell);
            };
            auto warp_cost = MeshQuality::nodeCost(*mesh, n2c_volume, n, warp_cost_function);
            cost = 0.8 * (cost) + 0.2 * warp_cost;
        }

        return cost;
    }
    double calcMaxNodeCost(int n) { return MeshQuality::nodeMaxHilbertCost(*mesh, n2c_volume, n); }

    Parfait::Point<double> interiorOptimize(int n) {
        auto p = mesh->mesh.points[n];
        auto delta_p = nodeStepDirection(n);
        delta_p *= node_step_length[n];
        p = p + node_step_length[n] * delta_p;
        return p;
    }

    Parfait::Point<double> surfaceSmoothAlgebraic(int n) {
        auto p = mesh->mesh.points[n];
        Parfait::Point<double> avg = {0, 0, 0};

        for (auto c : n2c_surface[n]) {
            auto cell = inf::Cell(*mesh, c);
            auto centroid = cell.averageCenter();
            avg += centroid;
        }

        avg /= double(n2c_surface[n].size());
        p = (1.0 - omega) * p + omega * avg;
        p = projection_surfaces.at(node_projection_surface_tag[n]).project(p);
        return p;
    }

    Parfait::Point<double> interiorSmoothAlgebraic(int n) {
        auto p = mesh->mesh.points[n];
        Parfait::Point<double> avg = {0, 0, 0};

        for (auto c : n2c_volume[n]) {
            auto cell = inf::Cell(*mesh, c);
            auto centroid = cell.averageCenter();
            avg += centroid;
        }

        avg /= double(n2c_volume[n].size());
        p = (1.0 - omega) * p + omega * avg;
        return p;
    }

    Parfait::Point<double> optimizeNode(int n) {
        if (algorithm != "smooth") {
            auto p = interiorOptimize(n);
            if (node_types[n] == NodeType::SURFACE) {
                p = projection_surfaces.at(node_projection_surface_tag[n]).project(p);
            }
            return p;
        } else {
            if (node_types[n] == NodeType::SURFACE) {
                return surfaceSmoothAlgebraic(n);
            }
            if (node_types[n] == NodeType::INTERIOR) {
                return interiorSmoothAlgebraic(n);
            }
        }
        return mesh->mesh.points[n];
    }

    void markNodesToMove() {
        node_mark = std::vector<int>(mesh->nodeCount(), 0);

        for (int c = 0; c < mesh->cellCount(); c++) {
            inf::Cell cell(*mesh, c);
            auto cost = MeshQuality::cellHilbertCost(cell.type(), cell.points());
            if (cost > hilbert_cost_threshold) {
                auto nodes = cell.nodes();
                for (auto n : nodes) {
                    node_mark[n] = 1.0;
                }
            }
        }
    }

    void smoothInteriorOnlyStep() {
        for (int n = 0; n < mesh->nodeCount(); n++) {
            if (mesh->nodeOwner(n) != mp.Rank()) continue;
            auto type = node_types[n];
            if (type == NodeType::INTERIOR) {
                mesh->mesh.points[n] = optimizeNode(n);
            }
        }
        Parfait::syncVector(mp, mesh->mesh.points, node_g2l, sync_pattern);
    }

    void smoothSurfaceOnlyStep() {
        for (int n = 0; n < mesh->nodeCount(); n++) {
            if (mesh->nodeOwner(n) != mp.Rank()) continue;
            auto type = node_types[n];
            if (type == NodeType::SURFACE) {
                mesh->mesh.points[n] = optimizeNode(n);
            }
        }
        Parfait::syncVector(mp, mesh->mesh.points, node_g2l, sync_pattern);
    }

    void smoothOnlyStep() {
        for (int n = 0; n < mesh->nodeCount(); n++) {
            if (mesh->nodeOwner(n) != mp.Rank()) continue;
            auto type = node_types[n];
            if (type == NodeType::SURFACE or type == NodeType::INTERIOR) {
                mesh->mesh.points[n] = optimizeNode(n);
            }
        }
        Parfait::syncVector(mp, mesh->mesh.points, node_g2l, sync_pattern);
    }

    void step() {
        if (smooth_only)
            smoothOnlyStep();
        else
            optimizeStep();
    }

    void optimizeStep() {
        int moved = 0;
        int jittered = 0;
        double max_node_cost = 0.0;
        for (int n = 0; n < mesh->nodeCount(); n++) {
            if (mesh->nodeOwner(n) != mp.Rank()) continue;
            if (node_mark[n] == 0) continue;
            auto type = node_types[n];
            if (type == NodeType::SURFACE or type == NodeType::INTERIOR) {
                auto cost = calcMaxNodeCost(n);
                if (cost > hilbert_cost_threshold) {
                    auto p = optimizeNode(n);
                    auto orig = mesh->mesh.points[n];
                    mesh->mesh.points[n] = p;
                    auto new_cost = calcMaxNodeCost(n);
                    if (new_cost > cost) {
                        max_node_cost = std::max(max_node_cost, cost);
                        mesh->mesh.points[n] = orig;
                        node_step_length[n] *= 0.5;
                        jittered++;
                    } else {
                        moved++;
                        node_step_length[n] *= 1.1;
                        if (new_cost < hilbert_cost_threshold) {
                            node_mark[n] = 0;
                        } else {
                            for (auto neighbor : n2n[n]) {
                                node_mark[neighbor] = 1;
                            }
                        }
                        max_node_cost = std::max(max_node_cost, new_cost);
                    }
                }
            }
        }

        max_node_cost = mp.ParallelMax(max_node_cost);
        Parfait::syncVector(mp, mesh->mesh.points, node_g2l, sync_pattern);

        moved = mp.ParallelSum(moved);
        jittered = mp.ParallelSum(jittered);
        // mp_rootprint(" Max Cost %e, Moved %d, jittered %d ", max_node_cost, moved, jittered);
    }

    double lineSearch(int n, Parfait::Point<double> direction) {
        direction.normalize();
        auto cost = calcNodeCost(n);
        double lo = 0.0;
        double hi = 2.0;
        double alpha = 0.5 * (hi + lo);

        auto p = mesh->mesh.points[n];

        for (int i = 0; i < 5; i++) {
            mesh->mesh.points[n] = p + alpha * node_step_length[n] * direction;
            auto new_cost = calcNodeCost(n);
            if (new_cost < cost) {
                lo = alpha;
            }
            if (new_cost > cost) {
                hi = alpha;
            }
            alpha = 0.5 * (lo + hi);
        }
        mesh->mesh.points[n] = p;
        return alpha;
    }

  public:
    MessagePasser mp;
    TinfMesh* mesh;
    Parfait::SyncPattern sync_pattern;
    std::map<long, int> node_g2l;
    double hilbert_cost_threshold = 0.5;
    double beta = 0.01;
    double omega = 0.1;
    std::string algorithm = "hilbert";
    bool optimize_warp = true;
    bool smooth_only = false;
    std::set<int> slip_tags;
    std::set<int> frozen_tags;
    std::vector<int> node_mark;
    std::vector<int> node_types;
    std::vector<int> node_projection_surface_tag;

    std::vector<std::vector<int>> n2n;
    std::vector<std::vector<int>> n2c_volume;
    std::vector<std::vector<int>> n2c_surface;
    std::vector<double> node_step_length;

    class ProjectionSurface {
      public:
        ProjectionSurface() {
            domain = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        }
        void addFacet(Parfait::Facet f) {
            Parfait::FacetSegment segment(f);
            facets.push_back(segment);
            Parfait::ExtentBuilder::add(domain, f[0]);
            Parfait::ExtentBuilder::add(domain, f[1]);
            Parfait::ExtentBuilder::add(domain, f[2]);
        }

        void finalize() {
            tree = std::make_shared<Parfait::DistanceTree>(domain);
            for (unsigned long i = 0; i < facets.size(); i++) {
                tree->insert(&facets[i]);
            }
            tree->finalize();
        }
        Parfait::Point<double> project(Parfait::Point<double> p) { return tree->closestPoint(p); }

      private:
        Parfait::Extent<double> domain;
        std::shared_ptr<Parfait::DistanceTree> tree;
        std::vector<Parfait::FacetSegment> facets;
    };

    std::map<int, ProjectionSurface> projection_surfaces;

    void buildProjectionSurfaces() {
        for (auto t : slip_tags) {
            ProjectionSurface s;
            for (int c = 0; c < mesh->cellCount(); c++) {
                auto type = mesh->cellType(c);
                if (mesh->is2DCellType(type) and t == mesh->cellTag(c)) {
                    inf::Cell cell(*mesh, c);
                    auto points = cell.points();
                    if (type == inf::MeshInterface::TRI_3) {
                        s.addFacet({points[0], points[1], points[2]});
                    } else if (type == inf::MeshInterface::QUAD_4) {
                        s.addFacet({points[0], points[1], points[2]});
                        s.addFacet({points[2], points[3], points[0]});
                    }
                }
            }
            projection_surfaces[t] = s;
            projection_surfaces[t].finalize();
        }
    }
    std::vector<double> calcNodalStepLength() {
        std::vector<double> step_size(mesh->nodeCount(), 0.0);

        for (int n = 0; n < mesh->nodeCount(); n++) {
            double avg_radius = 0.0;
            double max_radius = 0.0;
            auto p = mesh->mesh.points[n];
            for (auto neighbor : n2n[n]) {
                double distance = Parfait::Point<double>::distance(p, mesh->mesh.points[neighbor]);
                avg_radius += distance;
                max_radius = std::max(max_radius, distance);
            }
            avg_radius /= double(n2n[n].size());

            step_size[n] = avg_radius * beta;
        }

        return step_size;
    }
};
}
