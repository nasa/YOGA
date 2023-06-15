#pragma once
#include "InterpolationTools.h"
#include <parfait/ExtentBuilder.h>
#include <parfait/UnitTransformer.h>
#include <parfait/LeastSquaresReconstruction.h>
#include "LagrangeElement.h"

namespace YOGA {
template <typename T>
inline Parfait::UnitTransformer<T> getUnitTransformer(int n, const T* points, const T* query_point) {
    Parfait::Extent<T> e{{query_point}, {query_point}};
    for (int i = 0; i < n; i++) {
        auto p = &points[3 * i];
        for (int j = 0; j < 3; j++) {
            e.lo[j] = Linearize::min(e.lo[j], p[j]);
            e.hi[j] = Linearize::max(e.hi[j], p[j]);
        }
    }
    return Parfait::UnitTransformer<T>(e);
}

template <typename T>
bool areRstCoordinatesBonkers(const Parfait::Point<T>& rst) {
    T min_rst, max_rst;
    min_rst = max_rst = rst[0];
    min_rst = Linearize::min(min_rst, rst[1]);
    max_rst = Linearize::max(max_rst, rst[1]);
    min_rst = Linearize::min(min_rst, rst[2]);
    max_rst = Linearize::max(max_rst, rst[2]);
    for (int i = 0; i < 3; i++)
        if (std::isnan(Linearize::real(rst[i]))) return true;
    return Linearize::real(min_rst) < 0.0 or Linearize::real(max_rst) > 1.0;
}

template <typename T>
void FiniteElementInterpolation<T>::calcInverseDistanceWeights(int n,
                                                               const T* points,
                                                               const T* query_point,
                                                               T* weights) {
    T min_distance = 1.0e-14;
    auto unit_transformer = getUnitTransformer(n, points, query_point);
    Parfait::Point<T> q{query_point};
    q = unit_transformer.ToUnitSpace(q);
    T weight_sum = 0.0;
    for (int i = 0; i < n; i++) {
        Parfait::Point<T> b{&points[3 * i]};
        b = unit_transformer.ToUnitSpace(b);
        T distance = Linearize::max(min_distance, (b - q).magnitude());
        weights[i] = 1.0 / distance;
        weight_sum += weights[i];
    }
    // normalize
    T inv_weight_sum = 1.0;
    inv_weight_sum /= weight_sum;
    for (int i = 0; i < n; i++) {
        weights[i] *= inv_weight_sum;
    }
}

template <typename T>
void LeastSquaresInterpolation<T>::calcWeights(int n, const T* points, const T* query_point, T* weights) {
    using namespace Parfait;
    auto getNeighbor = [&](int i) { return Point<T>{points[3 * i], points[3 * i + 1], points[3 * i + 2]}; };
    Point<T> q{query_point[0], query_point[1], query_point[2]};
    DenseMatrix<T, Dynamic, Dynamic> coeffMatrix =
        Parfait::calcLLSQCoefficients(getNeighbor, n, 1, q, Parfait::LSQDimensionality::ThreeD);
    for (int i = 0; i < n; i++) weights[i] = coeffMatrix(i, 0);
}

template <typename T>
void FiniteElementInterpolation<T>::calcWeights(int n, const T* points, const T* query_point, T* weights) {
    switch (n) {
        case 4:
            calcWeightsTet(points, query_point, weights);
            break;
        case 5:
            calcWeightsPyramid(points, query_point, weights);
            break;
        case 6:
            calcWeightsPrism(points, query_point, weights);
            break;
        case 8:
            calcWeightsHex(points, query_point, weights);
            break;
        default:
            throw std::logic_error("Elements not supported for size " + std::to_string(n));
    }
}

template <typename T>
void FiniteElementInterpolation<T>::calcWeightsTet(const T* points, const T* query_point, T* weights) {
    std::array<Parfait::Point<T>, 4> vertices;
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = {points[3 * i], points[3 * i + 1], points[3 * i + 2]};
    }
    LagrangeTet<T> tet(vertices);
    auto rst = tet.calcComputationalCoordinates({query_point[0], query_point[1], query_point[2]});
    if (areRstCoordinatesBonkers(rst)) {
        calcInverseDistanceWeights(4, points, query_point, weights);
    } else {
        auto w = tet.evaluateBasis(rst[0], rst[1], rst[2]);
        for (size_t i = 0; i < vertices.size(); i++) {
            weights[i] = w[i];
        }
    }
}

template <typename T>
void FiniteElementInterpolation<T>::calcWeightsPyramid(const T* points, const T* query_point, T* weights) {
    std::array<Parfait::Point<T>, 5> vertices;
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = {points[3 * i], points[3 * i + 1], points[3 * i + 2]};
    }
    LagrangePyramid<T> pyramid(vertices);
    auto rst = pyramid.calcComputationalCoordinates({query_point[0], query_point[1], query_point[2]});
    if (areRstCoordinatesBonkers(rst)) {
        calcInverseDistanceWeights(5, points, query_point, weights);
    } else {
        auto w = pyramid.evaluateBasis(rst[0], rst[1], rst[2]);
        for (size_t i = 0; i < vertices.size(); i++) {
            weights[i] = w[i];
        }
    }
}

template <typename T>
void FiniteElementInterpolation<T>::calcWeightsPrism(const T* points, const T* query_point, T* weights) {
    std::array<Parfait::Point<T>, 6> vertices;
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = {points[3 * i], points[3 * i + 1], points[3 * i + 2]};
    }
    LagrangePrism<T> prism(vertices);
    auto rst = prism.calcComputationalCoordinates({query_point[0], query_point[1], query_point[2]});
    if (areRstCoordinatesBonkers(rst)) {
        calcInverseDistanceWeights(6, points, query_point, weights);
    } else {
        auto w = prism.evaluateBasis(rst[0], rst[1], rst[2]);
        for (size_t i = 0; i < vertices.size(); i++) {
            weights[i] = w[i];
        }
    }
}

template <typename T>
void FiniteElementInterpolation<T>::calcWeightsHex(const T* points, const T* query_point, T* weights) {
    std::array<Parfait::Point<T>, 8> vertices;
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = {points[3 * i], points[3 * i + 1], points[3 * i + 2]};
    }
    LagrangeHex<T> hex(vertices);
    auto rst = hex.calcComputationalCoordinates({query_point[0], query_point[1], query_point[2]});
    if (areRstCoordinatesBonkers(rst)) {
        calcInverseDistanceWeights(8, points, query_point, weights);
    } else {
        auto w = hex.evaluateBasis(rst[0], rst[1], rst[2]);
        for (size_t i = 0; i < vertices.size(); i++) {
            weights[i] = w[i];
        }
    }
}

template <typename T>
Parfait::Point<T> FiniteElementInterpolation<T>::calcComputationalCoordinatesTet(const T* points,
                                                                                 const T* query_point) {
    std::array<Parfait::Point<T>, 4> vertices;
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = {points[3 * i], points[3 * i + 1], points[3 * i + 2]};
    }
    LagrangeTet<T> tet(vertices);
    return tet.calcComputationalCoordinates({query_point[0], query_point[1], query_point[2]});
}

template <typename T>
Parfait::Point<T> FiniteElementInterpolation<T>::calcComputationalCoordinatesPyramid(const T* points,
                                                                                     const T* query_point) {
    std::array<Parfait::Point<T>, 5> vertices;
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = {points[3 * i], points[3 * i + 1], points[3 * i + 2]};
    }
    LagrangePyramid<T> pyramid(vertices);
    return pyramid.calcComputationalCoordinates({query_point[0], query_point[1], query_point[2]});
}

template <typename T>
Parfait::Point<T> FiniteElementInterpolation<T>::calcComputationalCoordinatesPrism(const T* points,
                                                                                   const T* query_point) {
    std::array<Parfait::Point<T>, 6> vertices;
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = {points[3 * i], points[3 * i + 1], points[3 * i + 2]};
    }
    LagrangePrism<T> prism(vertices);
    return prism.calcComputationalCoordinates({query_point[0], query_point[1], query_point[2]});
}

template <typename T>
Parfait::Point<T> FiniteElementInterpolation<T>::calcComputationalCoordinatesHex(const T* points,
                                                                                 const T* query_point) {
    std::array<Parfait::Point<T>, 8> vertices;
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = {points[3 * i], points[3 * i + 1], points[3 * i + 2]};
    }
    LagrangeHex<T> hex(vertices);
    return hex.calcComputationalCoordinates({query_point[0], query_point[1], query_point[2]});
}

template <typename T>
Parfait::Point<T> FiniteElementInterpolation<T>::calcComputationalCoordinates(int n,
                                                                              const T* points,
                                                                              const T* query_point) {
    switch (n) {
        case 4:
            return calcComputationalCoordinatesTet(points, query_point);
        case 5:
            return calcComputationalCoordinatesPyramid(points, query_point);
        case 6:
            return calcComputationalCoordinatesPrism(points, query_point);
        case 8:
            return calcComputationalCoordinatesHex(points, query_point);
        default:
            throw std::logic_error("Elements not supported for size " + std::to_string(n));
    }
}

}
