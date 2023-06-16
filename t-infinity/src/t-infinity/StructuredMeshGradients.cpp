#include "StructuredMeshGradients.h"
#include <parfait/LeastSquaresReconstruction.h>

inf::StructuredMeshLSQGradients::StructuredMeshLSQGradients(const inf::StructuredBlock& mesh)
    : dimensions(mesh.nodeDimensions()), weights(calcLeastSquaresEdgeWeightsPerStencil(mesh)) {}

inf::Vector3D<Parfait::Point<double>> inf::StructuredMeshLSQGradients::calcGradient(
    const inf::StructuredBlockField& field) const {
    Vector3D<Parfait::Point<double>> gradients(dimensions, {0, 0, 0});
    inf::loopMDRange({0, 0, 0}, dimensions, [&](int i, int j, int k) {
        auto& grad = gradients(i, j, k);
        auto center = field.value(i, j, k);
        int edge = 0;
        for (auto side : AllBlockSides) {
            auto ijk = getSide(i, j, k, side);
            if (inBounds(ijk[0], ijk[1], ijk[2])) {
                const auto& w = weights(i, j, k)[edge++];
                auto v = field.value(ijk[0], ijk[1], ijk[2]);
                grad[0] += w[0] * (v - center);
                grad[1] += w[1] * (v - center);
                grad[2] += w[2] * (v - center);
            }
        }
    });
    return gradients;
}
std::array<int, 3> inf::StructuredMeshLSQGradients::getSide(int i,
                                                            int j,
                                                            int k,
                                                            inf::BlockSide side) {
    switch (side) {
        case IMIN:
            return {i - 1, j, k};
        case JMIN:
            return {i, j - 1, k};
        case KMIN:
            return {i, j, k - 1};
        case IMAX:
            return {i + 1, j, k};
        case JMAX:
            return {i, j + 1, k};
        case KMAX:
            return {i, j, k + 1};
        default:
            PARFAIT_THROW("Unknown side: " + std::to_string(side));
    }
}

inf::Vector3D<inf::StructuredMeshLSQGradients::LSQCoefficients>
inf::StructuredMeshLSQGradients::calcLeastSquaresEdgeWeightsPerStencil(
    const inf::StructuredBlock& mesh) const {
    Vector3D<LSQCoefficients> stencil_weights(dimensions);

    std::vector<Parfait::Point<double>> edges(6);
    for (int k = 0; k < dimensions[2]; ++k) {
        for (int j = 0; j < dimensions[1]; ++j) {
            for (int i = 0; i < dimensions[0]; ++i) {
                auto center = mesh.point(i, j, k);
                int num_neighbors = 0;
                for (auto side : AllBlockSides) {
                    auto ijk = getSide(i, j, k, side);
                    if (inBounds(ijk[0], ijk[1], ijk[2])) {
                        edges[num_neighbors++] = mesh.point(ijk[0], ijk[1], ijk[2]) - center;
                    }
                }
                auto get_distance_to_neighbor = [&](int edge) -> Parfait::Point<double>& {
                    return edges[edge];
                };
                Parfait::LinearLSQ<double> lsq(Parfait::ThreeD, true);
                auto w =
                    lsq.calcLSQCoefficients(get_distance_to_neighbor, num_neighbors, 0.0, true);
                for (int l = 0; l < num_neighbors; ++l) {
                    stencil_weights(i, j, k).push_back({w(l, 0), w(l, 1), w(l, 2)});
                }
            }
        }
    }
    adjustPoleGradient(mesh, stencil_weights);
    return stencil_weights;
}

bool isPole(const std::array<Parfait::Point<double>, 4>& corners) {
    int matches = 0;
    for (int corner = 0; corner < 4; ++corner) {
        for (int c = 0; c < 4; ++c) {
            if (corner != c and corners[corner].approxEqual(corners[c], 1e-16)) {
                if (++matches == 4) return true;
            }
        }
    }
    return false;
}
void inf::StructuredMeshLSQGradients::adjustPoleGradient(
    const StructuredBlock& mesh, Vector3D<LSQCoefficients>& lsq_weights) const {
    for (auto side : AllBlockSides) {
        auto face = inf::getBlockFaces(mesh, side);
        if (isPole(face.corners)) {
            std::array<int, 3> start = {0, 0, 0};
            std::array<int, 3> end = mesh.nodeDimensions();
            switch (side) {
                case IMIN:
                case JMIN:
                case KMIN: {
                    auto ijk = sideToAxis(side);
                    end[ijk] = 1;
                    break;
                }
                case IMAX:
                case JMAX:
                case KMAX: {
                    auto ijk = sideToAxis(side);
                    start[ijk] = end[ijk] - 1;
                    break;
                }
                default:
                    PARFAIT_THROW("Unknown side: " + std::to_string(side));
            }
            for (int k = start[2]; k < end[2]; ++k) {
                for (int j = start[1]; j < end[1]; ++j) {
                    for (int i = start[0]; i < end[0]; ++i) {
                        auto center = mesh.point(i, j, k);
                        int e = 0;
                        for (auto s : AllBlockSides) {
                            auto ijk = getSide(i, j, k, s);
                            if (inBounds(ijk[0], ijk[1], ijk[2])) {
                                auto edge = mesh.point(ijk[0], ijk[1], ijk[2]) - center;
                                lsq_weights(i, j, k)[e] = {0, 0, 0};
                                for (int direction = 0; direction < 3; ++direction) {
                                    if (edge[direction] > 0)
                                        lsq_weights(i, j, k)[e][direction] = 1.0 / edge[direction];
                                }
                                e++;
                            }
                        }
                    }
                }
            }
        }
    }
}
bool inf::StructuredMeshLSQGradients::inBounds(int i, int j, int k) const {
    return i >= 0 and j >= 0 and k >= 0 and i < dimensions[0] and j < dimensions[1] and
           k < dimensions[2];
}
