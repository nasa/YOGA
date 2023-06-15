#pragma once
#include "StructuredBlockConnectivity.h"

namespace inf {
class TopologicalRotation {
  public:
    using AxisMapping = std::array<FaceAxisMapping, 3>;
    TopologicalRotation(AxisMapping host_to_neighbor_axis_mapping,
                        std::array<int, 3> neighbor_block_dimensions);
    std::tuple<int, int, int> operator()(int i, int j, int k) const;
    std::array<int, 3> convertHostToNeighbor(int i, int j, int k) const;

  private:
    AxisMapping host_to_neighbor_axis_mapping;
    std::array<int, 3> neighbor_dimensions;
};

}
