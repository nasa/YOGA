#include <RingAssertions.h>
#include <parfait/DenseMatrix.h>
#include <t-infinity/MetricManipulator.h>
#include "MockMeshes.h"
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/Extract.h>
#include <t-infinity/MetricAugmentationFactory.h>

using namespace inf;

TEST_CASE("Test the limiter augmentations") {
    Parfait::DenseMatrix<double, 3, 3> m = 50.0 * Tensor::Identity();
    double min_edge_length = 0.5;

    SECTION("Make sure the edges are too big") {
        auto decomposed = Parfait::MetricDecomposition::decompose(m);
        double target_edge_length = calcRadiusFromEigenvalue(decomposed.D(0, 0));
        REQUIRE(target_edge_length < min_edge_length);
    }

    SECTION("Limit them") {
        Parfait::Point<double> p{0, 0, 0};
        EdgeLengthLimiterAugmentation augmentation({3}, min_edge_length);

        REQUIRE_FALSE(augmentation.isActiveAt(p, 0));
        REQUIRE_FALSE(augmentation.isActiveAt(p, 1));
        REQUIRE_FALSE(augmentation.isActiveAt(p, 2));
        REQUIRE(augmentation.isActiveAt(p, 3));

        auto limited_metric = augmentation.targetMetricAt(p, 3, m);
        auto decomposed = Parfait::MetricDecomposition::decompose(limited_metric);
        auto target_edge_length = calcRadiusFromEigenvalue(decomposed.D(0, 0));
        REQUIRE(target_edge_length >= min_edge_length);
    }
}