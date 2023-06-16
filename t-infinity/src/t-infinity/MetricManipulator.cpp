#include <cmath>
#include "MetricManipulator.h"
#include "Extract.h"
#include "MetricAugmentationFactory.h"
#include "VectorFieldAdapter.h"
#include "MeshInquisitor.h"
#include "MeshHelpers.h"
#include "Cell.h"
#include <parfait/Plane.h>

namespace inf {

std::vector<Tensor> MetricManipulator::calcImpliedMetricAtCellsWithDimensionality(
    const inf::MeshInterface& mesh, int target_dimensionality) {
    using namespace Parfait;
    std::vector<Tensor> cell_metrics(mesh.cellCount(), Tensor::Identity());
    std::vector<int> cell;
    std::vector<Point<double>> vertices;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        mesh.cell(cell_id, cell);
        vertices.resize(cell.size());
        for (size_t i = 0; i < cell.size(); i++) vertices[i] = mesh.node(cell[i]);

        if (mesh.cellDimensionality(cell_id) == target_dimensionality) {
            if (inf::MeshInterface::TRI_3 == mesh.cellType(cell_id)) {
                cell_metrics[cell_id] = calcImpliedMetricForTriangle(vertices);
            } else if (inf::MeshInterface::TETRA_4 == mesh.cellType(cell_id)) {
                cell_metrics[cell_id] = calcImpliedMetricForTetrahedron(vertices);
            }
        }
    }
    return cell_metrics;
}

void calcCellWeights(inf::MeshInquisitor& inquisitor,
                     int dimensionality,
                     const std::vector<int>& cells,
                     std::vector<double>& weights) {
    weights.clear();
    double total_volume = 0.0;
    for (int nbr : cells) {
        double volume = dimensionality == 3 ? inquisitor.cellVolume(nbr) : inquisitor.cellArea(nbr);
        total_volume += volume;
        weights.push_back(volume);
    }
    double factor = 1.0 / total_volume;
    for (auto& w : weights) w *= factor;
}

void removeNeighborsWithLessThanTargetDimensionality(const inf::MeshInterface& mesh,
                                                     int target_dimensionality,
                                                     std::vector<std::vector<int>>& node_to_cell) {
    for (auto& nbrs : node_to_cell) {
        for (size_t i = 0; i < nbrs.size(); i++) {
            if (mesh.cellDimensionality(nbrs[i]) < target_dimensionality) {
                nbrs[i] = nbrs.back();
                nbrs.pop_back();
                i--;
            }
        }
    }
}

std::vector<Tensor> averageCellMetricToNodes(const inf::MeshInterface& mesh,
                                             int dimensionality,
                                             const std::vector<Tensor>& cell_metrics) {
    std::vector<Tensor> node_metrics(mesh.nodeCount());
    for (auto& m : node_metrics) m.clear();

    auto n2c = inf::NodeToCell::build(mesh);
    removeNeighborsWithLessThanTargetDimensionality(mesh, dimensionality, n2c);
    std::vector<double> weights;
    std::vector<Tensor> nbr_metrics;
    inf::MeshInquisitor inquisitor(mesh);
    for (int i = 0; i < mesh.nodeCount(); i++) {
        auto& nbrs = n2c[i];
        calcCellWeights(inquisitor, dimensionality, nbrs, weights);
        nbr_metrics.clear();
        for (int nbr : nbrs) {
            nbr_metrics.push_back(cell_metrics[nbr]);
        }
        node_metrics[i] = Parfait::MetricTensor::logEuclideanAverage(nbr_metrics, weights);
    }

    return node_metrics;
}

double calcMetricDensity(const Tensor& m) {
    auto decomp = Parfait::MetricDecomposition::decompose(m);
    double d = decomp.D(0, 0) * decomp.D(1, 1) * decomp.D(2, 2);
    if (d > 0.0)
        return std::sqrt(d);
    else
        return 0.0;
}

void to6ElementVector(const Tensor& m, double* dest) {
    dest[0] = m(0, 0);
    dest[1] = m(0, 1);
    dest[2] = m(0, 2);
    dest[3] = m(1, 1);
    dest[4] = m(1, 2);
    dest[5] = m(2, 2);
}

void to9ElementVector(const Tensor& m, double* dest) {
    dest[0] = m(0, 0);
    dest[1] = m(0, 1);
    dest[2] = m(0, 2);
    dest[3] = m(1, 0);
    dest[4] = m(1, 1);
    dest[5] = m(1, 2);
    dest[6] = m(2, 0);
    dest[7] = m(2, 1);
    dest[8] = m(2, 2);
}

std::vector<double> MetricManipulator::flattenToVector(const std::vector<Tensor>& metrics) {
    std::vector<double> v(metrics.size() * 6);
    for (size_t i = 0; i < metrics.size(); i++) {
        auto* p = v.data() + i * 6;
        to6ElementVector(metrics[i], p);
    }
    return v;
}

std::vector<double> MetricManipulator::flattenTo9ElementVector(const std::vector<Tensor>& metrics) {
    std::vector<double> v(metrics.size() * 9);
    for (size_t i = 0; i < metrics.size(); i++) {
        auto* p = v.data() + i * 9;
        to9ElementVector(metrics[i], p);
    }
    return v;
}

Tensor MetricManipulator::expandToMatrix(double* upper_triangular_metric) {
    double* m = upper_triangular_metric;
    Tensor metric;
    metric(0, 0) = m[0];
    metric(0, 1) = m[1];
    metric(0, 2) = m[2];
    metric(1, 1) = m[3];
    metric(1, 2) = m[4];
    metric(2, 2) = m[5];
    metric(1, 0) = metric(0, 1);
    metric(2, 0) = metric(0, 2);
    metric(2, 1) = metric(1, 2);
    return metric;
}

std::vector<Tensor> MetricManipulator::fromFieldToTensors(const FieldInterface& metric) {
    std::vector<Tensor> tensors(metric.size());
    std::array<double, 6> upper_triangular;
    for (int i = 0; i < metric.size(); i++) {
        metric.value(i, upper_triangular.data());
        tensors[i] = expandToMatrix(upper_triangular.data());
    }
    return tensors;
}

std::shared_ptr<inf::FieldInterface> MetricManipulator::limitMinimumEdgeLength(
    const MeshInterface& mesh,
    const FieldInterface& metric,
    const std::vector<int>& node_ids,
    double min_edge_length) {
    auto points = extractPoints(mesh);

    std::vector<std::shared_ptr<inf::Augmentation>> augmentations;
    augmentations.push_back(
        std::make_shared<EdgeLengthLimiterAugmentation>(node_ids, min_edge_length));

    return MetricManipulator::MetricManipulator::augment(points, metric, augmentations);
}

std::shared_ptr<inf::FieldInterface> MetricManipulator::toNodeField(
    const std::vector<Tensor>& node_metrics) {
    auto metric = MetricManipulator::flattenToVector(node_metrics);
    return std::make_shared<inf::VectorFieldAdapter>(
        "metric", inf::FieldAttributes::Node(), 6, metric);
}

std::shared_ptr<inf::FieldInterface> MetricManipulator::toNode9ElementHessianField(
    const std::vector<Tensor>& hessian) {
    std::vector<double> hessian_as_9_elements(9 * hessian.size());
    for (int n = 0; n < int(hessian.size()); n++) {
        auto H = hessian[n];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                hessian_as_9_elements[9 * n + 3 * i + j] = H(i, j);
            }
        }
    }
    return std::make_shared<inf::VectorFieldAdapter>(
        "hessian", inf::FieldAttributes::Node(), 9, hessian_as_9_elements);
}
std::shared_ptr<inf::FieldInterface> MetricManipulator::toNode9ElementHessianField(
    const std::vector<std::array<double, 6>>& hessian) {
    std::vector<double> hessian_as_9_elements(9 * hessian.size());
    std::array<int, 9> ij_to_i = {0, 1, 2, 1, 3, 4, 2, 4, 5};
    for (int n = 0; n < int(hessian.size()); n++) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                int index = ij_to_i[3 * i + j];
                hessian_as_9_elements[9 * n + 3 * i + j] = hessian[n][index];
            }
        }
    }
    return std::make_shared<inf::VectorFieldAdapter>(
        "hessian", inf::FieldAttributes::Node(), 9, hessian_as_9_elements);
}
std::shared_ptr<inf::FieldInterface> MetricManipulator::toRefineHessian(
    const std::vector<Tensor>& hessian) {
    // refine assumes 9 element fields are hessians.
    return toNode9ElementHessianField(hessian);
}
std::shared_ptr<inf::FieldInterface> MetricManipulator::toRefineHessian(
    const std::vector<std::array<double, 6>>& H) {
    // refine assumes 9 element fields are hessians.
    return toNode9ElementHessianField(H);
}
std::shared_ptr<inf::FieldInterface> MetricManipulator::calcImpliedMetricAtNodes(
    const inf::MeshInterface& mesh, int dimensionality) {
    using namespace Parfait;

    auto cell_metrics = calcImpliedMetricAtCellsWithDimensionality(mesh, dimensionality);
    auto node_metrics = averageCellMetricToNodes(mesh, dimensionality, cell_metrics);
    return toNodeField(node_metrics);
}

std::shared_ptr<inf::FieldInterface> MetricManipulator::calcImpliedMetricAtCells(
    const inf::MeshInterface& mesh, int dimensionality) {
    auto cell_metrics = calcImpliedMetricAtCellsWithDimensionality(mesh, dimensionality);
    auto metric = flattenToVector(cell_metrics);
    return std::make_shared<inf::VectorFieldAdapter>(
        "metric", inf::FieldAttributes::Cell(), 6, metric);
}

std::shared_ptr<inf::FieldInterface> MetricManipulator::calcUniformMetricAtNodes(
    const inf::MeshInterface& mesh) {
    std::vector<Tensor> uniform_metric(mesh.nodeCount(), Tensor::Identity());
    auto metric = flattenToVector(uniform_metric);
    return std::make_shared<inf::VectorFieldAdapter>(
        "metric", inf::FieldAttributes::Node(), 6, metric);
}

std::vector<double> calcCellVolumes(const inf::MeshInterface& mesh, int dimensionality) {
    std::vector<double> volumes(mesh.cellCount(), 0);
    inf::MeshInquisitor inquisitor(mesh);
    for (int i = 0; i < mesh.cellCount(); i++) {
        if (mesh.cellDimensionality(i) != dimensionality) continue;
        if (mesh.cellDimensionality(i) == 3) volumes[i] = inquisitor.cellVolume(i);
        if (mesh.cellDimensionality(i) == 2) volumes[i] = inquisitor.cellArea(i);
    }
    return volumes;
}

std::vector<double> calcNodeVolumes(const inf::MeshInterface& mesh, int dimensionality) {
    auto cell_volumes = calcCellVolumes(mesh, dimensionality);
    std::vector<double> node_volumes(mesh.nodeCount(), 0.0);
    inf::MeshInquisitor inquisitor(mesh);
    for (int i = 0; i < mesh.cellCount(); i++) {
        double volume_fraction = 1.0 / double(mesh.cellLength(i));
        for (int node : inquisitor.cell(i)) node_volumes[node] += volume_fraction * cell_volumes[i];
    }
    return node_volumes;
}

std::vector<double> extractDensities(const inf::FieldInterface& metric) {
    std::vector<double> densities(metric.size());
    std::array<double, 6> upper_triangular;
    for (int i = 0; i < metric.size(); i++) {
        metric.value(i, upper_triangular.data());
        densities[i] =
            calcMetricDensity(MetricManipulator::expandToMatrix(upper_triangular.data()));
    }
    return densities;
}

double MetricManipulator::calcComplexity(const MessagePasser& mp,
                                         const inf::MeshInterface& mesh,
                                         const inf::FieldInterface& metric) {
    auto densities = extractDensities(metric);
    double complexity = 0.0;

    std::vector<double> volumes;
    auto association = metric.association();

    auto dimensionality = maxCellDimensionality(mp, mesh);

    if (metric.association() == inf::FieldAttributes::Node()) {
        volumes = calcNodeVolumes(mesh, dimensionality);
        for (size_t i = 0; i < densities.size(); i++) {
            if (mesh.ownedNode(i)) complexity += densities[i] * volumes[i];
        }
    } else if (metric.association() == inf::FieldAttributes::Cell()) {
        volumes = calcCellVolumes(mesh, dimensionality);
        for (size_t i = 0; i < densities.size(); i++) {
            if (mesh.ownedCell(i)) complexity += densities[i] * volumes[i];
        }
    } else {
        PARFAIT_THROW("Cannot calculate complexity of metric with unknown association");
    }

    return mp.ParallelSum(complexity);
}

std::shared_ptr<inf::FieldInterface> MetricManipulator::scaleToTargetComplexity(
    const MessagePasser& mp,
    const inf::MeshInterface& mesh,
    const inf::FieldInterface& metric,
    double target_complexity) {
    double current_complexity = calcComplexity(mp, mesh, metric);
    double scaling_factor = target_complexity / current_complexity;
    scaling_factor = std::cbrt(scaling_factor * scaling_factor);

    std::vector<Tensor> scaled_metric(metric.size());
    std::array<double, 6> upper_triangular;
    for (int i = 0; i < metric.size(); i++) {
        metric.value(i, upper_triangular.data());
        auto m = expandToMatrix(upper_triangular.data());
        auto decomp = Parfait::MetricDecomposition::decompose(m);
        for (int j = 0; j < 3; j++) decomp.D(j, j) *= scaling_factor;
        scaled_metric[i] = Parfait::MetricDecomposition::recompose(decomp);
    }
    auto flattened = flattenToVector(scaled_metric);
    return std::make_shared<inf::VectorFieldAdapter>("metric", metric.association(), 6, flattened);
}

Tensor applyAugmentationsToTensor(const Tensor& current_metric,
                                  const Parfait::Point<double>& p,
                                  int id,
                                  const std::vector<std::shared_ptr<Augmentation>>& augmentations) {
    std::vector<Tensor> metrics;
    metrics.push_back(current_metric);
    for (auto& aug : augmentations) {
        if (aug->isActiveAt(p, id)) metrics.push_back(aug->targetMetricAt(p, id, current_metric));
    }
    double weight = 1.0 / double(metrics.size());
    std::vector<double> weights(metrics.size(), weight);
    return Parfait::MetricTensor::logEuclideanAverage(metrics, weights);
}

Tensor intersectTensorWithAugmentations(
    const Tensor& current_metric,
    const Parfait::Point<double>& p,
    int id,
    const std::vector<std::shared_ptr<Augmentation>>& augmentations) {
    auto metric = current_metric;
    for (auto& aug : augmentations) {
        if (aug->isActiveAt(p, id)) {
            auto aug_metric = aug->targetMetricAt(p, id, current_metric);
            metric = Parfait::MetricTensor::intersect(metric, aug_metric);
        }
    }
    return metric;
}

std::shared_ptr<inf::FieldInterface> MetricManipulator::augment(
    const std::vector<Parfait::Point<double>>& points,
    const inf::FieldInterface& metric,
    const std::vector<std::shared_ptr<Augmentation>>& augmentations) {
    std::vector<Tensor> augmented(metric.size());
    std::array<double, 6> upper_triangular;
    for (int i = 0; i < metric.size(); i++) {
        metric.value(i, upper_triangular.data());
        auto m = expandToMatrix(upper_triangular.data());
        augmented[i] = applyAugmentationsToTensor(m, points[i], i, augmentations);
    }

    auto flattened = flattenToVector(augmented);
    return std::make_shared<inf::VectorFieldAdapter>(
        metric.name(), metric.association(), 6, flattened);
}

std::shared_ptr<inf::FieldInterface> MetricManipulator::overrideMetric(
    const std::vector<Parfait::Point<double>>& points,
    const inf::FieldInterface& metric,
    const std::vector<std::shared_ptr<Augmentation>>& augmentations) {
    auto augmented = fromFieldToTensors(metric);
    for (int i = 0; i < metric.size(); i++) {
        for (auto& aug : augmentations) {
            if (aug->isActiveAt(points[i], i))
                augmented[i] = aug->targetMetricAt(points[i], i, augmented[i]);
        }
    }
    return toNodeField(augmented);
}

std::shared_ptr<inf::FieldInterface> MetricManipulator::intersect(
    const inf::FieldInterface& metric_a, const inf::FieldInterface& metric_b) {
    std::vector<Tensor> intersections(metric_a.size());
    auto A = fromFieldToTensors(metric_a);
    auto B = fromFieldToTensors(metric_b);
    for (int i = 0; i < metric_a.size(); i++)
        intersections[i] = Parfait::MetricTensor::intersect(A[i], B[i]);
    return toNodeField(intersections);
}

std::shared_ptr<inf::FieldInterface> MetricManipulator::logEuclideanAverage(
    const std::vector<std::shared_ptr<inf::FieldInterface>>& metric_fields,
    const std::vector<double>& weights) {
    double total = std::accumulate(weights.begin(), weights.end(), 0.0);
    std::vector<double> normalized_weights = weights;
    std::for_each(
        normalized_weights.begin(), normalized_weights.end(), [&](double& w) { w = w / total; });
    std::vector<std::vector<Tensor>> tensors;
    for (auto& field : metric_fields) tensors.emplace_back(fromFieldToTensors(*field));
    std::vector<Tensor> avg(tensors.front().size());
    for (int i = 0; i < int(avg.size()); i++) {
        std::vector<Tensor> to_average;
        for (int j = 0; j < int(tensors.size()); j++) to_average.emplace_back(tensors[j][i]);
        avg[i] = Parfait::MetricTensor::logEuclideanAverage(to_average, normalized_weights);
    }
    return toNodeField(avg);
}

std::shared_ptr<inf::FieldInterface> MetricManipulator::intersect(
    const std::vector<Parfait::Point<double>>& points,
    const inf::FieldInterface& metric,
    const std::vector<std::shared_ptr<Augmentation>>& augmentations) {
    std::vector<Tensor> augmented(metric.size());
    std::array<double, 6> upper_triangular;
    for (int i = 0; i < metric.size(); i++) {
        metric.value(i, upper_triangular.data());
        auto m = expandToMatrix(upper_triangular.data());
        augmented[i] = intersectTensorWithAugmentations(m, points[i], i, augmentations);
    }

    auto flattened = flattenToVector(augmented);
    return std::make_shared<inf::VectorFieldAdapter>("metric", metric.association(), 6, flattened);
}
void MetricManipulator::setPrescribedSpacingOnMetric(std::vector<inf::Tensor>& metric,
                                                     MessagePasser mp,
                                                     const MeshInterface& mesh,
                                                     int tag,
                                                     double target_spacing) {
    auto boundary_cell_dimensionality = maxCellDimensionality(mp, mesh) - 1;
    PARFAIT_ASSERT(boundary_cell_dimensionality == 2,
                   "prescribed metric spacing not implemented for 2D");
    auto nodes_on_tag = inf::extractOwnedNodeIdsOn2DTags(mesh, tag);

    auto cell_ids_on_tag = inf::extractCellIdsOnTag(mesh, tag, boundary_cell_dimensionality);

    auto normals =
        inf::calcSurfaceAreaAtNodes(mesh, {cell_ids_on_tag.begin(), cell_ids_on_tag.end()});
    for (auto& n : normals) n.normalize();

    double error_norm = 0.0;
    for (int node_id : nodes_on_tag) {
        Parfait::Plane<double> p(normals[node_id]);
        Parfait::Point<double> t1, t2;
        std::tie(t1, t2) = p.orthogonalVectors();

        auto normal = p.unit_normal * target_spacing;
        t1 *= 4.0 * target_spacing;
        t2 *= 4.0 * target_spacing;

        auto h = Parfait::MetricTensor::edgeLength(metric[node_id], normal);
        double e = 1.0 / h;
        error_norm += e * e;

        metric[node_id] = Parfait::MetricTensor::metricFromEllipse(normal, t1, t2);
    }

    error_norm =
        std::sqrt(mp.ParallelSum(error_norm) / double(mp.ParallelSum(nodes_on_tag.size())));
    mp_rootprint(
        "Tag: <%s> L2 norm of wall normal spacing: %e\n", mesh.tagName(tag).c_str(), error_norm);
}
std::shared_ptr<inf::FieldInterface> MetricManipulator::setPrescribedSpacingOnMetric(
    MessagePasser mp,
    const MeshInterface& mesh,
    const FieldInterface& metric,
    const std::set<int>& tags,
    double target_complexity,
    double target_spacing) {
    std::shared_ptr<FieldInterface> prescribed_metric;

    std::vector<Tensor> new_metric(metric.size());
    for (int node_id = 0; node_id < metric.size(); ++node_id)
        new_metric[node_id] = MetricManipulator::expandToMatrix(metric.at(node_id).data());

    for (int pass = 0; pass < 10; ++pass) {
        for (int tag : tags)
            setPrescribedSpacingOnMetric(new_metric, mp, mesh, tag, target_spacing);
        prescribed_metric = toNodeField(new_metric);

        auto last_complexity = MetricManipulator::calcComplexity(mp, mesh, *prescribed_metric);

        if (last_complexity / target_complexity > 100.0) {
            mp_rootprint(
                "[%d/10] Unable to meet target complexity.  Increasing target spacing from %e to "
                "%e\n",
                pass + 1,
                target_spacing,
                2.0 * target_spacing);
            target_spacing *= 2.0;
        } else {
            prescribed_metric = MetricManipulator::scaleToTargetComplexity(
                mp, mesh, *prescribed_metric, target_complexity);

            for (int node_id = 0; node_id < metric.size(); ++node_id)
                new_metric[node_id] =
                    MetricManipulator::expandToMatrix(prescribed_metric->at(node_id).data());
            mp_rootprint(
                "[%d/10] Current complexity: %e Target Complexity: %e Target spacing: %e\n",
                pass + 1,
                last_complexity,
                target_complexity,
                target_spacing);
        }
    }
    return prescribed_metric;
}

std::shared_ptr<inf::FieldInterface> expand6ElementVectorToTensor(
    const inf::FieldInterface& six_element_field) {
    std::vector<Tensor> output_field(six_element_field.size());

    int num_rows = six_element_field.size();
    std::array<double, 6> buffer;
    for (int i = 0; i < num_rows; i++) {
        six_element_field.value(i, buffer.data());
        output_field[i] = MetricManipulator::expandToMatrix(buffer.data());
    }

    return MetricManipulator::toNodeField(output_field);
}
}
