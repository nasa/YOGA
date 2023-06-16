#include "TopologicalRotation.h"
inf::TopologicalRotation::TopologicalRotation(
    inf::TopologicalRotation::AxisMapping host_to_neighbor_axis_mapping,
    std::array<int, 3> neighbor_block_dimensions)
    : host_to_neighbor_axis_mapping(host_to_neighbor_axis_mapping),
      neighbor_dimensions(neighbor_block_dimensions) {}

std::tuple<int, int, int> inf::TopologicalRotation::operator()(int i, int j, int k) const {
    auto ijk = convertHostToNeighbor(i, j, k);
    return {ijk[0], ijk[1], ijk[2]};
}
std::array<int, 3> inf::TopologicalRotation::convertHostToNeighbor(int i, int j, int k) const {
    std::array<int, 3> host_ijk = {i, j, k};
    std::array<int, 3> neighbor_ijk = {i, j, k};
    auto setNeighborIJK = [&](BlockAxis host_axis) -> void {
        if (host_to_neighbor_axis_mapping[host_axis].direction == ReversedDirection) {
            auto neighbor_axis = host_to_neighbor_axis_mapping[host_axis].axis;
            neighbor_ijk[neighbor_axis] =
                neighbor_dimensions[neighbor_axis] - 1 - host_ijk[host_axis];
        } else {
            neighbor_ijk[host_to_neighbor_axis_mapping[host_axis].axis] = host_ijk[host_axis];
        }
    };
    setNeighborIJK(I);
    setNeighborIJK(J);
    setNeighborIJK(K);

    return neighbor_ijk;
}