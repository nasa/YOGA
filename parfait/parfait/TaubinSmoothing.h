#pragma once
#include <parfait/STL.h>
#include <parfait/STLFactory.h>
#include <parfait/StitchMesh.h>

namespace Parfait {
class TaubinSmoothing {
  public:
    static inline void taubinSmoothing(const std::vector<int>& node_ids,
                                       std::vector<Parfait::Point<double>>& points,
                                       const std::vector<double>& damping_weights,
                                       int passes = 5) {
        auto n2n = Parfait::STL::buildNodeToNode(node_ids, points.size());
        taubinSmoothing(Parfait::Point<double>{0, 0, 0}, n2n, points, damping_weights, passes);
    }
    static inline void taubinSmoothing(const std::vector<int>& node_ids,
                                       std::vector<Parfait::Point<double>>& points,
                                       int passes = 5) {
        std::vector<double> damping_weights(points.size(), 1.0);
        auto n2n = Parfait::STL::buildNodeToNode(node_ids, points.size());
        taubinSmoothing(Parfait::Point<double>{0, 0, 0}, n2n, points, damping_weights, passes);
    }

    template <typename Graph, typename T>
    static inline void taubinSmoothing(
        T zero, const Graph& n2n, std::vector<T>& field, const std::vector<double>& damping_weights, int passes = 5) {
        for (int i = 0; i < passes; i++) {
            taubinSmoothingPass(zero, n2n, field, damping_weights, 0.33);
            taubinSmoothingPass(zero, n2n, field, damping_weights, -0.34);
        }
    }

    template <typename Graph, typename T>
    static inline void taubinSmoothing(T zero, const Graph& n2n, std::vector<T>& field, int passes = 5) {
        std::vector<double> damping_weights(field.size(), 1.0);
        taubinSmoothing(zero, n2n, field, damping_weights, passes);
    }

    template <typename Neighbors, typename T>
    static T calcDelta(T zero, const std::vector<T>& points, const Neighbors& n2n, int node) {
        T delta = zero;
        for (auto neighbor : n2n) {
            delta += points[node] - points[neighbor];
        }
        delta /= double(n2n.size());
        return delta;
    }

    template <typename Graph, typename T>
    static void taubinSmoothingPass(
        T zero, const Graph& n2n, std::vector<T>& field, const std::vector<double>& damping_weights, double coeff) {
        std::vector<T> deltas(n2n.size(), zero);
        for (int node = 0; node < long(n2n.size()); node++) {
            deltas[node] = calcDelta(zero, field, n2n[node], node);
        }

        for (int node = 0; node < long(n2n.size()); node++) {
            field[node] += coeff * damping_weights[node] * deltas[node];
        }
    }
};
}
