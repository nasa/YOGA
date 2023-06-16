#pragma once
#include <ddata/ETD.h>

namespace YOGA {
template <size_t N>
class DifferentiableDonorElement {
  public:
    typedef Linearize::ETD<N> Number;
    static constexpr int VertexCount = (N - 3) / 3;

    DifferentiableDonorElement(const double* v, const double* p, int n) {
        query_point = convert(p, VertexCount);
        for (int i = 0; i < n; i++) {
            vertices[i] = convert(&v[3 * i], i);
        }
    }

    static double vertexPartialX(Number x, int vertex_id) { return x.dx(3 * vertex_id); }

    static double vertexPartialY(Number x, int vertex_id) { return x.dx(3 * vertex_id + 1); }

    static double vertexPartialZ(Number x, int vertex_id) { return x.dx(3 * vertex_id + 2); }

    static double queryPointPartialX(Number x) {
        int query_point_index = VertexCount;
        return x.dx(3 * query_point_index);
    }

    static double queryPointPartialY(Number x) {
        int query_point_index = VertexCount;
        return x.dx(3 * query_point_index + 1);
    }

    static double queryPointPartialZ(Number x) {
        int query_point_index = VertexCount;
        return x.dx(3 * query_point_index + 2);
    }

    std::array<Parfait::Point<Number>, VertexCount> vertices;
    Parfait::Point<Number> query_point;

  private:
    static Parfait::Point<Number> convert(const double* from, int dependence) {
        Parfait::Point<Number> p{from[0], from[1], from[2]};
        setDependence(p, dependence);
        return p;
    }

    static void setDependence(Parfait::Point<Number>& p, int index) {
        for (int i = 0; i < 3; i++) p[i].set_derivative(1.0, 3 * index + i);
    }
};

class DifferentiableDonorElementFactory {
  public:
    template <size_t N>
    static DifferentiableDonorElement<(N + 1) * 3> make(const std::array<Parfait::Point<double>, N>& v,
                                                        const Parfait::Point<double>& p) {
        return DifferentiableDonorElement<(N + 1) * 3>(v.front().data(), p.data(), N);
    }
};
}