#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/SMesh.h>
#include <t-infinity/StructuredMeshGradients.h>

TEST_CASE("Compute node gradients on structured mesh via LSQ") {
    auto block = inf::CartesianBlock({0, 0, 0}, {1, 1, 1}, {10, 10, 10}, 0);
    auto dimensions = block.nodeDimensions();
    auto linear_field = inf::SField::createBlock(dimensions);
    Parfait::Point<double> expected_gradient = {1.1, 2.2, 3.3};
    for (int k = 0; k < dimensions[2]; ++k) {
        for (int j = 0; j < dimensions[1]; ++j) {
            for (int i = 0; i < dimensions[0]; ++i) {
                const auto& p = block.point(i, j, k);
                linear_field->values(i, j, k) = Parfait::Point<double>::dot(p, expected_gradient);
            }
        }
    }
    auto gradient_calculator = inf::StructuredMeshLSQGradients(block);
    auto gradients = gradient_calculator.calcGradient(*linear_field);
    for (int k = 0; k < dimensions[2]; ++k) {
        for (int j = 0; j < dimensions[1]; ++j) {
            for (int i = 0; i < dimensions[0]; ++i) {
                auto actual_gradient = gradients(i, j, k);
                INFO("expected: " + expected_gradient.to_string() + " actual: " + actual_gradient.to_string());
                REQUIRE(expected_gradient.approxEqual(actual_gradient, 1e-12));
            }
        }
    }
}