#pragma once
#include "MetricManipulator.h"
#include "MeshConnectivity.h"
#include "GhostSyncer.h"
#include "MeshHelpers.h"
#include "Extract.h"
#include <parfait/LeastSquaresReconstruction.h>
#include <parfait/Plane.h>
#include <parfait/SetTools.h>

namespace inf {
inline double calcRadiusFromEigenvalue(double eig) { return 1.0 / std::sqrt(eig); }

class EdgeLengthLimiterAugmentation : public inf::Augmentation {
  public:
    EdgeLengthLimiterAugmentation(std::vector<int> ids, double h)
        : ids(ids.begin(), ids.end()), min_edge_length(h), limited_eig(1.0 / (h * h)) {}
    virtual bool isActiveAt(const Parfait::Point<double>& p, int id) const { return ids.count(id); }
    virtual inf::Tensor targetMetricAt(const Parfait::Point<double>& p,
                                       int id,
                                       const inf::Tensor& current_metric) const {
        auto decomposed = Parfait::MetricDecomposition::decompose(current_metric);
        for (int i = 0; i < 3; i++) {
            double r = calcRadiusFromEigenvalue(decomposed.D(i, i));
            if (r < min_edge_length) decomposed.D(i, i) = limited_eig;
        }
        return Parfait::MetricDecomposition::recompose(decomposed);
    }

  private:
    std::set<int> ids;
    double min_edge_length;
    double limited_eig;
};

class DistanceBlendedMetric : public Augmentation {
  public:
    DistanceBlendedMetric(double wall_distance_threshold,
                          const std::vector<double>& node_wall_distances,
                          const std::vector<Tensor>& metric_near_wall,
                          double blending_power = 1.0)
        : wall_distance_threshold(wall_distance_threshold),
          blending_power(blending_power),
          node_wall_distances(node_wall_distances),
          metric_near_wall(metric_near_wall) {}
    bool isActiveAt(const Parfait::Point<double>& p, int id) const override {
        return node_wall_distances[id] < wall_distance_threshold;
    }
    Tensor targetMetricAt(const Parfait::Point<double>& p,
                          int id,
                          const Tensor& current_metric) const override {
        auto w = blendingFunction(node_wall_distances[id]);
        return Parfait::MetricTensor::logEuclideanAverage({metric_near_wall[id], current_metric},
                                                          {w, 1.0 - w});
    }

  private:
    double wall_distance_threshold;
    double blending_power;
    const std::vector<double> node_wall_distances;

    std::vector<Tensor> metric_near_wall;

    double blendingFunction(double wall_distance) const {
        return std::pow(wall_distance_threshold - wall_distance, blending_power) /
               std::pow(wall_distance_threshold, blending_power);
    }
};

class ExplicitWallSpacingMetricBlending : public Augmentation {
  public:
    ExplicitWallSpacingMetricBlending(double target_wall_spacing,
                                      double growth_rate,
                                      double wall_distance_threshold,
                                      double tangent_spacing,
                                      const std::vector<double>& node_wall_distances,
                                      MessagePasser mp,
                                      std::shared_ptr<MeshInterface> mesh)
        : h_wall(target_wall_spacing),
          growth_factor(growth_rate),
          wall_distance_threshold(wall_distance_threshold),
          tangent_spacing(tangent_spacing),
          syncer(mp, mesh),
          node_wall_distances(node_wall_distances),
          wall_distance_grad(calcGradients(mp, mesh)),
          implied_metric(calcImpliedMetric(mp, *mesh)) {
        overrideSurfaceGradients(mp, *mesh);
        syncer.syncNodes(wall_distance_grad);
    }
    bool isActiveAt(const Parfait::Point<double>& p, int id) const override {
        return node_wall_distances[id] < wall_distance_threshold;
    }
    Tensor targetMetricAt(const Parfait::Point<double>& p,
                          int id,
                          const Tensor& current_metric) const override {
        using namespace Parfait;
        Plane<double> plane(wall_distance_grad[id]);
        auto h = spacingFunction(node_wall_distances[id]);
        auto n = plane.unit_normal * h;
        Point<double> t1, t2;
        std::tie(t1, t2) = plane.orthogonalVectors();
        if (tangent_spacing > 0.0) {
            t1 *= tangent_spacing;
            t2 *= tangent_spacing;
        } else {
            double ratio1 = MetricTensor::edgeLength(current_metric, t1);
            double ratio2 = MetricTensor::edgeLength(current_metric, t2);

            t1 /= std::max(ratio1, 1e-12);
            t2 /= std::max(ratio2, 1e-12);
        }
        auto result = MetricTensor::metricFromEllipse(n, t1, t2);
        if (not std::isfinite(result.norm())) {
            printf("Error in explicit wall spacing function\n");
            printf("wall distance: %e h: %e wall-normal: %s\n",
                   node_wall_distances[id],
                   h,
                   wall_distance_grad[id].to_string().c_str());
            printf("Ellipse: n <%s> t1 <%s> t2 <%s>\n",
                   n.to_string().c_str(),
                   t1.to_string().c_str(),
                   t2.to_string().c_str());
            printf("metric: \n");
            result.print(1e-15);
            result = implied_metric[id];
        }
        return result;
    }

  private:
    double h_wall;
    double growth_factor;
    double wall_distance_threshold;
    double tangent_spacing;
    GhostSyncer syncer;
    std::vector<double> node_wall_distances;
    std::vector<Parfait::Point<double>> wall_distance_grad;
    std::vector<Tensor> implied_metric;

    double spacingFunction(double wall_distance) const {
        double h_final = wall_distance_threshold * std::min(1.0, growth_factor / 4.0);
        double b = std::atanh(h_wall / wall_distance_threshold);
        double a = std::atanh(h_final / wall_distance_threshold);
        return wall_distance_threshold * std::tanh(a * wall_distance / wall_distance_threshold + b);
    }

    std::vector<Parfait::Point<double>> calcGradients(MessagePasser mp,
                                                      std::shared_ptr<MeshInterface> mesh) {
        syncer.syncNodes(node_wall_distances);

        std::vector<Parfait::Point<double>> wall_normal_gradient(mesh->nodeCount(), {0, 0, 0});
        auto n2n = NodeToNode::build(*mesh);
        auto dimensionality =
            static_cast<Parfait::LSQDimensionality>(maxCellDimensionality(mp, *mesh));
        for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
            if (node_wall_distances[node_id] >= wall_distance_threshold) continue;
            const auto& stencil = n2n[node_id];
            Parfait::Point<double> center = mesh->node(node_id);
            auto get_distance = [&](int n) -> Parfait::Point<double> {
                Parfait::Point<double> xyz = mesh->node(stencil[n]);
                return xyz - center;
            };
            Parfait::LinearLSQ<double> lsq(dimensionality, true);
            auto coefficients = lsq.calcLSQCoefficients(get_distance, stencil.size(), -1);

            for (int row = 0; row < coefficients.rows(); ++row) {
                auto ddist = node_wall_distances[stencil[row]] - node_wall_distances[node_id];
                for (int col = 0; col < dimensionality; ++col) {
                    wall_normal_gradient[node_id][col] += coefficients(row, col) * ddist;
                }
            }
        }
        return wall_normal_gradient;
    }

    std::vector<Tensor> calcImpliedMetric(MessagePasser mp, const MeshInterface& mesh) const {
        auto m = MetricManipulator::calcImpliedMetricAtNodes(mesh, maxCellDimensionality(mp, mesh));
        return MetricManipulator::fromFieldToTensors(*m);
    }

    void overrideSurfaceGradients(MessagePasser mp, const MeshInterface& mesh) {
        auto surface_cell_dimensionality = maxCellDimensionality(mp, mesh) - 1;
        std::set<int> cell_ids;
        for (int cell_id = 0; cell_id < mesh.cellCount(); ++cell_id) {
            if (mesh.cellDimensionality(cell_id) == surface_cell_dimensionality) {
                auto cell_nodes = mesh.cell(cell_id);
                bool wall_dist_is_zero = true;
                for (int n : cell_nodes)
                    if (node_wall_distances[n] > 0.0) wall_dist_is_zero = false;
                if (wall_dist_is_zero) cell_ids.insert(cell_id);
            }
        }
        auto surface_area = calcSurfaceAreaAtNodes(mesh, cell_ids);
        for (int node_id = 0; node_id < mesh.nodeCount(); ++node_id) {
            if (node_wall_distances[node_id] == 0.0)
                wall_distance_grad[node_id] = surface_area[node_id];
        }
    }
};

class ShrinkMultiscale : public Augmentation {
  public:
    ShrinkMultiscale(double target_wall_spacing,
                     double growth_rate,
                     double wall_distance_threshold,
                     double tangent_spacing,
                     const std::vector<double>& node_wall_distances,
                     MessagePasser mp,
                     std::shared_ptr<MeshInterface> mesh)
        : h_wall(target_wall_spacing),
          growth_factor(growth_rate),
          wall_distance_threshold(wall_distance_threshold),
          tangent_spacing(tangent_spacing),
          syncer(mp, mesh),
          node_wall_distances(node_wall_distances),
          wall_distance_grad(calcGradients(mp, mesh)),
          implied_metric(calcImpliedMetric(mp, *mesh)) {
        overrideSurfaceGradients(mp, *mesh);
        syncer.syncNodes(wall_distance_grad);
    }
    bool isActiveAt(const Parfait::Point<double>& p, int id) const override {
        return node_wall_distances[id] < wall_distance_threshold;
    }
    Tensor targetMetricAt(const Parfait::Point<double>& p,
                          int id,
                          const Tensor& current_metric) const override {
        using namespace Parfait;
        Plane<double> plane(wall_distance_grad[id]);
        auto h_target = spacingFunction(node_wall_distances[id]);
        if (tangent_spacing > 0) {
            auto ratio = MetricTensor::edgeLength(current_metric, plane.unit_normal);
            auto h_current = 1.0 / ratio;

            auto result = current_metric;
            if (h_target < h_current) result *= std::pow(h_current / h_target, 2);
            return result;
        } else {
            auto decomp = MetricDecomposition::decompose(current_metric);
            auto most_aligned = getMostAligned(plane.unit_normal, decomp);
            auto h_current = 1.0 / std::sqrt(decomp.D(most_aligned, most_aligned));
            if (h_target < h_current)
                decomp.D(most_aligned, most_aligned) = 1.0 / std::pow(h_target, 2);
            return MetricDecomposition::recompose(decomp);
        }
    }

  private:
    double h_wall;
    double growth_factor;
    double wall_distance_threshold;
    double tangent_spacing;
    GhostSyncer syncer;
    std::vector<double> node_wall_distances;
    std::vector<Parfait::Point<double>> wall_distance_grad;
    std::vector<Tensor> implied_metric;

    double spacingFunction(double wall_distance) const {
        auto exponent = std::min(wall_distance, wall_distance_threshold) / h_wall;
        exponent = std::min(1000.0, exponent);
        return h_wall * std::pow(growth_factor, exponent);
    }

    int getMostAligned(const Parfait::Point<double>& direction,
                       const Parfait::MetricDecomposition::Decomposition& decomp) const {
        int most_aligned = -1;
        double most_aligned_dot_product = -1.0;
        for (int i = 0; i < 3; ++i) {
            auto e = Parfait::MetricTensor::extractEigenvector(decomp, i);
            Parfait::Point<double> eigenvector = {e[0], e[1], e[2]};
            eigenvector.normalize();
            auto dp = std::abs(direction.dot(eigenvector));
            if (dp > most_aligned_dot_product) {
                most_aligned_dot_product = dp;
                most_aligned = i;
            }
        }
        PARFAIT_ASSERT(most_aligned_dot_product > -1.0 and most_aligned > -1,
                       "Failed to find most aligned");
        return most_aligned;
    }

    std::vector<Parfait::Point<double>> calcGradients(MessagePasser mp,
                                                      std::shared_ptr<MeshInterface> mesh) {
        syncer.syncNodes(node_wall_distances);

        std::vector<Parfait::Point<double>> wall_normal_gradient(mesh->nodeCount(), {0, 0, 0});
        auto n2n = NodeToNode::build(*mesh);
        auto dimensionality =
            static_cast<Parfait::LSQDimensionality>(maxCellDimensionality(mp, *mesh));
        for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
            if (node_wall_distances[node_id] >= wall_distance_threshold) continue;
            const auto& stencil = n2n[node_id];
            Parfait::Point<double> center = mesh->node(node_id);
            auto get_distance = [&](int n) -> Parfait::Point<double> {
                Parfait::Point<double> xyz = mesh->node(stencil[n]);
                return xyz - center;
            };
            Parfait::LinearLSQ<double> lsq(dimensionality, true);
            auto coefficients = lsq.calcLSQCoefficients(get_distance, stencil.size(), -1);

            for (int row = 0; row < coefficients.rows(); ++row) {
                auto ddist = node_wall_distances[stencil[row]] - node_wall_distances[node_id];
                for (int col = 0; col < dimensionality; ++col) {
                    wall_normal_gradient[node_id][col] += coefficients(row, col) * ddist;
                }
            }
        }
        return wall_normal_gradient;
    }

    std::vector<Tensor> calcImpliedMetric(MessagePasser mp, const MeshInterface& mesh) const {
        auto m = MetricManipulator::calcImpliedMetricAtNodes(mesh, maxCellDimensionality(mp, mesh));
        return MetricManipulator::fromFieldToTensors(*m);
    }

    void overrideSurfaceGradients(MessagePasser mp, const MeshInterface& mesh) {
        auto surface_cell_dimensionality = maxCellDimensionality(mp, mesh) - 1;
        std::set<int> cell_ids;
        for (int cell_id = 0; cell_id < mesh.cellCount(); ++cell_id) {
            if (mesh.cellDimensionality(cell_id) == surface_cell_dimensionality) {
                auto cell_nodes = mesh.cell(cell_id);
                bool wall_dist_is_zero = true;
                for (int n : cell_nodes)
                    if (node_wall_distances[n] > 0.0) wall_dist_is_zero = false;
                if (wall_dist_is_zero) cell_ids.insert(cell_id);
            }
        }
        auto surface_area = calcSurfaceAreaAtNodes(mesh, cell_ids);
        for (int node_id = 0; node_id < mesh.nodeCount(); ++node_id) {
            if (node_wall_distances[node_id] == 0.0)
                wall_distance_grad[node_id] = surface_area[node_id];
        }
    }
};

class ApplySpacingAtTags : public Augmentation {
  public:
    ApplySpacingAtTags(const std::set<int>& tags,
                       double target_wall_spacing,
                       MessagePasser mp,
                       std::shared_ptr<MeshInterface> mesh)
        : h_wall(target_wall_spacing), syncer(mp, mesh) {
        auto dimensionality = maxCellDimensionality(mp, *mesh);

        std::set<int> cells_on_tags;
        for (int tag : tags) {
            auto nodes = extractNodeIdsViaDimensionalityAndTag(*mesh, tag, dimensionality - 1);
            nodes_on_boundaries.insert(nodes.begin(), nodes.end());
            auto cell_ids = extractCellIdsOnTag(*mesh, tag, dimensionality - 1);
            cells_on_tags.insert(cell_ids.begin(), cell_ids.end());
        }
        surface_normals = calcSurfaceAreaAtNodes(*mesh, cells_on_tags);
    }
    bool isActiveAt(const Parfait::Point<double>& p, int id) const override {
        return nodes_on_boundaries.count(id) > 0;
    }
    Tensor targetMetricAt(const Parfait::Point<double>& p,
                          int id,
                          const Tensor& current_metric) const override {
        using namespace Parfait;
        auto n = surface_normals.at(id);
        n.normalize();
        auto ratio = MetricTensor::edgeLength(current_metric, n);
        auto h_current = 1.0 / ratio;

        auto result = current_metric;
        if (h_wall < h_current) result *= std::pow(h_current / h_wall, 2);
        return result;
    }

  private:
    double h_wall;
    GhostSyncer syncer;
    std::set<int> nodes_on_boundaries;
    std::vector<Parfait::Point<double>> surface_normals;
};

class ClipEigenvalues : public Augmentation {
  public:
    ClipEigenvalues(std::vector<Tensor> target_metric) : target_metric(target_metric) {}
    bool isActiveAt(const Parfait::Point<double>& p, int id) const override { return true; }
    Tensor targetMetricAt(const Parfait::Point<double>& p,
                          int id,
                          const Tensor& current_metric) const override {
        using namespace Parfait;
        auto target_decomp = MetricDecomposition::decompose(target_metric[id]);
        auto source_decomp = MetricDecomposition::decompose(current_metric);
        for (int i = 0; i < 3; ++i) {
            target_decomp.D(i, i) = std::max(target_decomp.D(i, i), source_decomp.D(i, i));
        }
        return MetricDecomposition::recompose(target_decomp);
    }

  private:
    std::vector<Tensor> target_metric;
};
}