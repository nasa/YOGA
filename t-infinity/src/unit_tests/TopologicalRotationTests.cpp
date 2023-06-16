#include <RingAssertions.h>
#include <t-infinity/TopologicalRotation.h>

using namespace inf;

TEST_CASE("Can perform topological rotation") {
    std::array<int, 3> neighbor_block_dimensions = {9, 5, 5};

    int host_i = 1;
    int host_j = 2;
    int host_k = 3;

    SECTION("Same axis + same direction") {
        std::array<FaceAxisMapping, 3> axis_mapping{};
        for (auto axis : {BlockAxis::I, BlockAxis::J, BlockAxis::K}) {
            axis_mapping[axis].axis = axis;
            axis_mapping[axis].direction = AxisDirection::SameDirection;
        }

        auto topological_rotation = TopologicalRotation(axis_mapping, neighbor_block_dimensions);
        int i, j, k;

        std::tie(i, j, k) = topological_rotation(host_i, host_j, host_k);
        REQUIRE(host_i == i);
        REQUIRE(host_j == j);
        REQUIRE(host_k == k);
    }
    SECTION("Same axis + reversed direction") {
        std::array<FaceAxisMapping, 3> axis_mapping{};
        for (auto axis : {BlockAxis::I, BlockAxis::J, BlockAxis::K}) {
            axis_mapping[axis].axis = axis;
            axis_mapping[axis].direction = AxisDirection::ReversedDirection;
        }

        auto topological_rotation = TopologicalRotation(axis_mapping, neighbor_block_dimensions);

        int neighbor_i, neighbor_j, neighbor_k;
        std::tie(neighbor_i, neighbor_j, neighbor_k) = topological_rotation(host_i, host_j, host_k);
        REQUIRE(neighbor_block_dimensions[I] - 1 - host_i == neighbor_i);
        REQUIRE(neighbor_block_dimensions[J] - 1 - host_j == neighbor_j);
        REQUIRE(neighbor_block_dimensions[K] - 1 - host_k == neighbor_k);
    }
    SECTION("Different axis + same direction") {
        std::array<FaceAxisMapping, 3> axis_mapping{};
        for (auto axis : {BlockAxis::I, BlockAxis::J, BlockAxis::K}) {
            axis_mapping[axis].direction = AxisDirection::SameDirection;
        }
        axis_mapping[BlockAxis::I].axis = BlockAxis::J;
        axis_mapping[BlockAxis::J].axis = BlockAxis::K;
        axis_mapping[BlockAxis::K].axis = BlockAxis::I;

        auto topological_rotation = TopologicalRotation(axis_mapping, neighbor_block_dimensions);

        int neighbor_i, neighbor_j, neighbor_k;
        std::tie(neighbor_i, neighbor_j, neighbor_k) = topological_rotation(host_i, host_j, host_k);
        REQUIRE(host_k == neighbor_i);
        REQUIRE(host_i == neighbor_j);
        REQUIRE(host_j == neighbor_k);
    }
    SECTION("Different axis + reversed direction") {
        std::array<FaceAxisMapping, 3> host_to_neighbor_axis_mapping{};
        for (auto axis : {BlockAxis::I, BlockAxis::J, BlockAxis::K}) {
            host_to_neighbor_axis_mapping[axis].direction = AxisDirection::ReversedDirection;
        }
        host_to_neighbor_axis_mapping[BlockAxis::I].axis = BlockAxis::J;
        host_to_neighbor_axis_mapping[BlockAxis::J].axis = BlockAxis::K;
        host_to_neighbor_axis_mapping[BlockAxis::K].axis = BlockAxis::I;

        auto topological_rotation = TopologicalRotation(host_to_neighbor_axis_mapping, neighbor_block_dimensions);

        int neighbor_i, neighbor_j, neighbor_k;
        std::tie(neighbor_i, neighbor_j, neighbor_k) = topological_rotation(host_i, host_j, host_k);
        REQUIRE(neighbor_block_dimensions[J] - 1 - host_i == neighbor_j);
        REQUIRE(neighbor_block_dimensions[K] - 1 - host_j == neighbor_k);
        REQUIRE(neighbor_block_dimensions[I] - 1 - host_k == neighbor_i);
    }
}