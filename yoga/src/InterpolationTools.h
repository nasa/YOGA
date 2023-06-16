#pragma once
#include <parfait/Point.h>

namespace YOGA {

std::array<double, 4> least_squares_plane_coefficients(int n, const double* points, const double* solution_values);

double least_squares_interpolate(int n, const double* points, const double* solutions, const double* query_point);


class BarycentricInterpolation {
  public:
    static void calcWeightsTet(const double* tet,const double* p, double* weights);

    static std::array<double, 4> calculateBarycentricCoordinates(const std::array<Parfait::Point<double>, 4>& tet,
                                                          const Parfait::Point<double>& p);
};

template <typename T>
class LeastSquaresInterpolation {
  public:
    static void calcWeights(int n, const T* points, const T* query_point, T* weights);
};

template <typename T>
class FiniteElementInterpolation {
  public:
    static void calcInverseDistanceWeights(int n, const T* points, const T* query_point, T* weights);
    static void calcWeights(int n, const T* points, const T* query_point, T* weights);
    static void calcWeightsTet(const T* points, const T* query_point, T* weights);
    static void calcWeightsPyramid(const T* points, const T* query_point, T* weights);
    static void calcWeightsPrism(const T* points, const T* query_point, T* weights);
    static void calcWeightsHex(const T* points, const T* query_point, T* weights);

    static Parfait::Point<T> calcComputationalCoordinates(int n, const T* points, const T* query_point);
    static Parfait::Point<T> calcComputationalCoordinatesTet(const T* points, const T* query_point);
    static Parfait::Point<T> calcComputationalCoordinatesPyramid(const T* points, const T* query_point);
    static Parfait::Point<T> calcComputationalCoordinatesPrism(const T* points, const T* query_point);
    static Parfait::Point<T> calcComputationalCoordinatesHex(const T* points, const T* query_point);
};

}
