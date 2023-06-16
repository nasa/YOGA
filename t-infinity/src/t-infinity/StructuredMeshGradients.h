#pragma once

#include <t-infinity/MDVector.h>
#include "StructuredBlockConnectivity.h"

namespace inf {
class StructuredMeshLSQGradients {
  public:
    StructuredMeshLSQGradients() = default;
    StructuredMeshLSQGradients(const StructuredBlock& mesh);
    Vector3D<Parfait::Point<double>> calcGradient(const StructuredBlockField& field) const;

  private:
    typedef std::vector<std::array<double, 3>> LSQCoefficients;
    std::array<int, 3> dimensions;
    Vector3D<LSQCoefficients> weights;

    static std::array<int, 3> getSide(int i, int j, int k, BlockSide side);
    Vector3D<LSQCoefficients> calcLeastSquaresEdgeWeightsPerStencil(
        const StructuredBlock& mesh) const;
    void adjustPoleGradient(const StructuredBlock& mesh,
                            Vector3D<LSQCoefficients>& lsq_weights) const;
    bool inBounds(int i, int j, int k) const;
};
}
