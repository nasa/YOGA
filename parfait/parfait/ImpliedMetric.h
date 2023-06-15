#pragma once
#include <parfait/DenseMatrix.h>
#include <parfait/CGNSElements.h>

namespace Parfait {
template <typename Tri>
DenseMatrix<double, 3, 3> calcImpliedMetricForTriangle(const Tri& tri) {
    DenseMatrix<double, 3, 3> A;
    Vector<double, 3> b;
    for (int i = 0; i < 3; i++) b[i] = 1.0;
    auto edges = CGNS::Triangle::edge_to_node;
    for (size_t i = 0; i < edges.size(); i++) {
        auto p0 = tri[edges[i][0]];
        auto p1 = tri[edges[i][1]];
        auto dx = p1[0] - p0[0];
        auto dy = p1[1] - p0[1];
        A(i, 0) = dx * dx;
        A(i, 1) = 2 * dx * dy;
        A(i, 2) = dy * dy;
    }
    auto M = GaussJordan::solve(A, b);

    DenseMatrix<double, 3, 3> matrix{{M[0], M[1], 0.0}, {M[1], M[2], 0.0}, {0.0, 0.0, 1.0}};
    return matrix;
}
template <typename Tet>
DenseMatrix<double, 3, 3> calcImpliedMetricForTetrahedron(const Tet& tet) {
    DenseMatrix<double, 6, 6> A;
    Vector<double, 6> b;
    for (int i = 0; i < 6; i++) b[i] = 1.0;
    auto edges = CGNS::Tet::edge_to_node;
    for (size_t i = 0; i < edges.size(); i++) {
        auto p0 = tet[edges[i][0]];
        auto p1 = tet[edges[i][1]];
        auto dx = p1[0] - p0[0];
        auto dy = p1[1] - p0[1];
        auto dz = p1[2] - p0[2];
        A(i, 0) = dx * dx;
        A(i, 1) = 2 * dx * dy;
        A(i, 2) = 2 * dx * dz;
        A(i, 3) = dy * dy;
        A(i, 4) = 2 * dy * dz;
        A(i, 5) = dz * dz;
    }
    auto M = GaussJordan::solve(A, b);

    DenseMatrix<double, 3, 3> matrix{{M[0], M[1], M[2]}, {M[1], M[3], M[4]}, {M[2], M[4], M[5]}};
    return matrix;
}
}