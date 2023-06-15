#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/DenseMatrix.h>
#include <parfait/ImpliedMetric.h>
#include <parfait/MetricTensor.h>
#include <t-infinity/MeshConnectivity.h>
#include <parfait/Metrics.h>
#include <t-infinity/FieldInterface.h>

namespace inf {

using Tensor = Parfait::DenseMatrix<double, 3, 3>;

class Augmentation {
  public:
    virtual bool isActiveAt(const Parfait::Point<double>& p, int id) const = 0;
    virtual Tensor targetMetricAt(const Parfait::Point<double>& p,
                                  int id,
                                  const Tensor& current_metric) const = 0;
    virtual ~Augmentation() = default;
};

class MetricManipulator {
  public:
    static std::shared_ptr<inf::FieldInterface> calcImpliedMetricAtNodes(
        const inf::MeshInterface& mesh, int dimensionality);
    static std::shared_ptr<inf::FieldInterface> calcImpliedMetricAtCells(
        const inf::MeshInterface& mesh, int dimensionality);
    static std::shared_ptr<inf::FieldInterface> calcUniformMetricAtNodes(
        const inf::MeshInterface& mesh);
    static double calcComplexity(const MessagePasser& mp,
                                 const inf::MeshInterface& mesh,
                                 const inf::FieldInterface& metric);
    static std::shared_ptr<inf::FieldInterface> scaleToTargetComplexity(
        const MessagePasser& mp,
        const inf::MeshInterface& mesh,
        const inf::FieldInterface& metric,
        double target_complexity);
    static std::shared_ptr<inf::FieldInterface> augment(
        const std::vector<Parfait::Point<double>>& points,
        const inf::FieldInterface& metric,
        const std::vector<std::shared_ptr<Augmentation>>& augmentations);
    static std::shared_ptr<inf::FieldInterface> overrideMetric(
        const std::vector<Parfait::Point<double>>& points,
        const inf::FieldInterface& metric,
        const std::vector<std::shared_ptr<Augmentation>>& augmentations);

    static std::shared_ptr<inf::FieldInterface> intersect(const inf::FieldInterface& metric_a,
                                                          const inf::FieldInterface& metric_b);
    static std::shared_ptr<inf::FieldInterface> intersect(
        const std::vector<Parfait::Point<double>>& points,
        const inf::FieldInterface& metric,
        const std::vector<std::shared_ptr<Augmentation>>& augmentations);

    static std::shared_ptr<inf::FieldInterface> logEuclideanAverage(
        const std::vector<std::shared_ptr<inf::FieldInterface>>& metric_fields,
        const std::vector<double>& weights);

    static Tensor expandToMatrix(double* upper_triangular_metric);
    static std::vector<double> flattenToVector(const std::vector<Tensor>& metrics);
    static std::vector<double> flattenTo9ElementVector(const std::vector<Tensor>& metrics);

    static std::shared_ptr<FieldInterface> limitMinimumEdgeLength(const MeshInterface& mesh,
                                                                  const FieldInterface& metric,
                                                                  const std::vector<int>& node_ids,
                                                                  double min_edge_length);
    static void setPrescribedSpacingOnMetric(std::vector<Tensor>& metric,
                                             MessagePasser mp,
                                             const MeshInterface& mesh,
                                             int tag,
                                             double target_spacing);

    static std::shared_ptr<FieldInterface> setPrescribedSpacingOnMetric(
        MessagePasser mp,
        const MeshInterface& mesh,
        const FieldInterface& metric,
        const std::set<int>& tags,
        double target_complexity,
        double target_spacing);

    static std::vector<Tensor> fromFieldToTensors(const FieldInterface& H);
    static std::shared_ptr<inf::FieldInterface> toNodeField(const std::vector<Tensor>& H);
    static std::shared_ptr<inf::FieldInterface> toNode9ElementHessianField(
        const std::vector<Tensor>& metric);
    static std::shared_ptr<inf::FieldInterface> toNode9ElementHessianField(
        const std::vector<std::array<double, 6>>& H);
    static std::shared_ptr<inf::FieldInterface> toRefineHessian(const std::vector<Tensor>& H);
    static std::shared_ptr<inf::FieldInterface> toRefineHessian(
        const std::vector<std::array<double, 6>>& H);

  private:
    static std::vector<Tensor> calcImpliedMetricAtCellsWithDimensionality(
        const MeshInterface& mesh, int target_dimensionality);
};

std::shared_ptr<inf::FieldInterface> expand6ElementVectorToTensor(
    const inf::FieldInterface& six_element_field);

void to6ElementVector(const Tensor& m, double* dest);

double calcMetricDensity(const Tensor& m);
}
