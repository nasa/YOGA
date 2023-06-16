#include <RingAssertions.h>
#include <t-infinity/MetricManipulator.h>
#include <t-infinity/MetricAugmentationFactory.h>

using namespace inf;

TEST_CASE("Distance-based metric augmentation") {
    double threshold = 0.1;
    std::vector<double> distances = {0.0, 0.05, 0.5, 1.0};

    std::vector<Tensor> metric_near_wall(distances.size(), Tensor::Identity());
    std::vector<Tensor> metric_away_from_wall(distances.size(), 2.0 * Tensor::Identity());

    DistanceBlendedMetric augmentation(threshold, distances, metric_near_wall);
    REQUIRE(augmentation.isActiveAt({0, 0, 0}, 0));
    REQUIRE(augmentation.isActiveAt({0, 0, 0}, 1));
    REQUIRE_FALSE(augmentation.isActiveAt({0, 0, 0}, 2));
    REQUIRE_FALSE(augmentation.isActiveAt({0, 0, 0}, 3));

    auto averaged = augmentation.targetMetricAt({0, 0, 0}, 0, metric_away_from_wall[0]);
    REQUIRE(1.0 == Approx(Parfait::MetricTensor::edgeLength(averaged, {1, 0, 0})));

    auto expected_tensor =
        Parfait::MetricTensor::logEuclideanAverage({metric_near_wall[1], metric_away_from_wall[1]}, {0.5, 0.5});
    auto expected_length = Parfait::MetricTensor::edgeLength(expected_tensor, {1, 0, 0});

    averaged = augmentation.targetMetricAt({0, 0, 0}, 1, metric_away_from_wall[1]);
    REQUIRE(expected_length == Approx(Parfait::MetricTensor::edgeLength(averaged, {1, 0, 0})));
}