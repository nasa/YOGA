#pragma once
#include "Point.h"
#include "Throw.h"
#include "DenseMatrix.h"
#include "Decompositions.h"
#include <complex>

namespace Parfait {

template <typename getPoint>
int detectStencilDimension(getPoint get_neighbor_point, int number_of_neighbors);
void checkForInvalidCoeffsAndSetGradCoefsToZero(Parfait::DenseMatrix<double>& coefficients);

enum LSQDimensionality { OneD = 1, TwoD = 2, ThreeD = 3 };

template <typename T>
class LSQCoefficients {
  public:
    using Matrix = DenseMatrix<T, Dynamic, Dynamic>;
    virtual int numberOfCoefficients() const = 0;
    virtual void setRow(Matrix& A, int point, const Point<T>& distance) const = 0;

    template <typename getDistance>
    auto calcLSQCoefficients(getDistance get_distance,
                             int number_of_neighbors,
                             double distance_weighting_power,
                             bool throw_on_nan = true) {
        auto A = buildMatrix(get_distance, number_of_neighbors, distance_weighting_power);
        auto householder_qr = HouseholderQR<Matrix>(A);
        Matrix Ainv = householder_qr.calcPseudoInverse();
        auto coefficients = buildLSQCoefficientsFromInverse(
            Ainv, get_distance, number_of_neighbors, distance_weighting_power, throw_on_nan);
        if (throw_on_nan) checkForNaNAndThrow(number_of_neighbors, A, Ainv, coefficients);
        return coefficients;
    }

    template <typename getDistance>
    auto buildLSQCoefficientsFromInverse(const Matrix& Ainv,
                                         getDistance get_distance,
                                         int number_of_neighbors,
                                         double distance_weighting_power,
                                         bool throw_on_nan = true) -> DenseMatrix<T, Dynamic, Dynamic> {
        Matrix coefficients(number_of_neighbors, numberOfCoefficients());
        for (int neighbor = 0; neighbor < number_of_neighbors; ++neighbor) {
            auto distance = get_distance(neighbor);
            auto w = calcDistanceWeight(distance.magnitude(), distance_weighting_power);
            for (int i = 0; i < numberOfCoefficients(); ++i) {
                coefficients(neighbor, i) = w * Ainv(i, neighbor);
            }
        }
        return coefficients;
    }

    template <typename getDistance>
    auto buildMatrix(getDistance get_distance, int number_of_neighbors, double distance_weighting_power) {
        Matrix A(number_of_neighbors, numberOfCoefficients());
        for (int neighbor = 0; neighbor < number_of_neighbors; ++neighbor) {
            auto distance = get_distance(neighbor);
            auto w = calcDistanceWeight(distance.magnitude(), distance_weighting_power);
            setRow(A, neighbor, distance);
            for (int col = 0; col < A.cols(); ++col) A(neighbor, col) *= w;
        }
        return A;
    }

    template <typename Matrix>
    static void checkForNaNAndThrow(int number_of_neighbors,
                                    const Matrix& A,
                                    const Matrix& Ainv,
                                    const Matrix& coefficients) {
        bool good = true;
        for (int p = 0; p < number_of_neighbors; p++) {
            for (int i = 0; i < coefficients.cols(); i++) {
                using namespace std;
                if (not std::isfinite(real(coefficients(p, i)))) {
                    printf("LSQ coefficients make no sense:\n");
                    printf("coeff:");
                    for (int c = 0; c < coefficients.cols(); ++c) {
                        printf(" %e", real(coefficients(p, c)));
                    }
                    printf("\n");
                    printf("A: \n");
                    A.print();
                    printf("Ainv: \n");
                    Ainv.print();
                    good = false;
                    break;
                }
            }
        }

        if (not good) {
            PARFAIT_WARNING("Could not create sensible nodal LSQ stencils");
        }
    }
    static T calcDistanceWeight(T distance, double power) {
        using namespace std;
        return distance > std::numeric_limits<double>::epsilon() or power > 0 ? T(pow(distance, power)) : T(1.0);
    }
    virtual ~LSQCoefficients() = default;
};

template <typename T>
class LinearLSQ : public LSQCoefficients<T> {
  public:
    using Matrix = typename LSQCoefficients<T>::Matrix;
    LinearLSQ(LSQDimensionality dimensionality, bool gradient_only)
        : dimensionality(dimensionality), shift(gradient_only ? 0 : 1) {}
    int numberOfCoefficients() const override { return dimensionality + shift; }
    void setRow(Matrix& A, int point, const Point<T>& distance) const override {
        for (int i = 0; i < shift; ++i) A(point, i) = 1.0;
        for (int i = 0; i < dimensionality; ++i) A(point, i + shift) = distance[i];
    }
    LSQDimensionality dimensionality;
    int shift;
};

template <typename T>
class QuadraticLSQ : public LSQCoefficients<T> {
  public:
    using typename LSQCoefficients<T>::Matrix;
    QuadraticLSQ(LSQDimensionality dimensionality, bool gradient_hessian_only)
        : dimensionality(dimensionality), shift(gradient_hessian_only ? 0 : 1) {}
    int numberOfCoefficients() const override {
        switch (dimensionality) {
            case OneD:
                return 2 + shift;
            case TwoD:
                return 5 + shift;
            case ThreeD:
                return 9 + shift;
            default:
                PARFAIT_THROW("Invalid dimensionality");
        }
    }
    void setRow(Matrix& A, int point, const Point<T>& distance) const override {
        const auto& dx = distance[0];
        const auto& dy = distance[1];
        const auto& dz = distance[2];
        for (int i = 0; i < shift; ++i) A(point, i) = 1.0;
        switch (dimensionality) {
            case OneD: {
                A(point, 0 + shift) = dx;
                A(point, 1 + shift) = 0.5 * dx * dx;
                break;
            }
            case TwoD: {
                A(point, 0 + shift) = dx;
                A(point, 1 + shift) = dy;
                A(point, 2 + shift) = 0.5 * dx * dx;
                A(point, 3 + shift) = dx * dy;
                A(point, 4 + shift) = 0.5 * dy * dy;
                break;
            }
            case ThreeD: {
                A(point, 0 + shift) = dx;
                A(point, 1 + shift) = dy;
                A(point, 2 + shift) = dz;
                A(point, 3 + shift) = 0.5 * dx * dx;
                A(point, 4 + shift) = dx * dy;
                A(point, 5 + shift) = dx * dz;
                A(point, 6 + shift) = 0.5 * dy * dy;
                A(point, 7 + shift) = dy * dz;
                A(point, 8 + shift) = 0.5 * dz * dz;
                break;
            }
            default:
                PARFAIT_THROW("Invalid dimensionality");
        }
    }
    LSQDimensionality dimensionality;
    int shift;
};

template <typename getPoint, typename T>
auto calcLLSQCoefficients(getPoint get_neighbor_point,
                          int number_of_neighbors,
                          double distance_weighting_power,
                          const Point<T>& center_point,
                          LSQDimensionality dimensionality,
                          bool throw_on_nan = true) {
    auto get_distance = [&](int n) { return get_neighbor_point(n) - center_point; };
    LinearLSQ<T> lsq(dimensionality, false);
    return lsq.calcLSQCoefficients(get_distance, number_of_neighbors, distance_weighting_power, throw_on_nan);
}

template <typename getPoint, typename T>
auto calcLLSQGradientCoefficients(getPoint get_neighbor_point,
                                  int number_of_neighbors,
                                  double distance_weighting_power,
                                  const Point<T>& center_point,
                                  LSQDimensionality dimensionality,
                                  bool throw_on_nan = true) {
    auto get_distance = [&](int n) { return get_neighbor_point(n) - center_point; };
    LinearLSQ<T> lsq(dimensionality, true);
    return lsq.calcLSQCoefficients(get_distance, number_of_neighbors, distance_weighting_power, throw_on_nan);
}

template <typename getPoint, typename T>
auto calcQLSQCoefficients(getPoint get_neighbor_point,
                          int number_of_neighbors,
                          double distance_weighting_power,
                          const Point<T>& center_point,
                          LSQDimensionality dimensionality,
                          bool throw_on_nan = true) {
    auto get_distance = [&](int n) { return get_neighbor_point(n) - center_point; };
    QuadraticLSQ<T> lsq(dimensionality, false);
    return lsq.calcLSQCoefficients(get_distance, number_of_neighbors, distance_weighting_power, throw_on_nan);
}

template <typename getPoint, typename T>
auto calcQLSQGradientHessianCoefficients(getPoint get_neighbor_point,
                                         int number_of_neighbors,
                                         double distance_weighting_power,
                                         const Point<T>& center_point,
                                         LSQDimensionality dimensionality,
                                         bool throw_on_nan = true) {
    auto get_distance = [&](int n) { return get_neighbor_point(n) - center_point; };
    QuadraticLSQ<T> lsq(dimensionality, true);
    return lsq.calcLSQCoefficients(get_distance, number_of_neighbors, distance_weighting_power, throw_on_nan);
}

template <typename getPoint>
int detectStencilDimension(getPoint get_neighbor_point, int number_of_neighbors) {
    std::array<double, 3> max_delta = {0, 0, 0};
    auto first_point = get_neighbor_point(0);
    for (int point = 0; point < number_of_neighbors; ++point) {
        auto dx = get_neighbor_point(point) - first_point;
        for (int e = 0; e < 3; e++) {
            max_delta[e] = std::max(std::abs(dx[e]), max_delta[e]);
        }
    }
    int dimension = 3;
    if (max_delta[2] < 1.0e-16) dimension = 2;
    return dimension;
}

inline void checkForInvalidCoeffsAndSetGradCoefsToZero(Parfait::DenseMatrix<double>& coefficients, int dimension) {
    bool good = true;
    for (int p = 0; p < coefficients.rows(); p++) {
        for (int i = 0; i < dimension + 1; i++) {
            // Only check the 0th and 1st derivative coefficients
            int num_cols_to_check = std::min(4, coefficients.cols());
            for (int i = 0; i < num_cols_to_check; i++) {
                if (std::isnan(coefficients(p, i))) {
                    printf("LSQ coefficients make no sense:\n");
                    printf("coeff: ");
                    for (int d = 0; d < coefficients.cols(); d++) {
                        printf("%e ", coefficients(p, d));
                    }
                    good = false;
                    break;
                }
            }
        }
    }

    if (not good) {
        PARFAIT_WARNING(
            "Could not create sensible nodal LSQ stencils.  Cell dropping to 1st "
            "order.");
        for (int r = 0; r < coefficients.rows(); r++) {
            for (int d = 0; d < dimension; d++) {
                coefficients(r, d + 1) = 0.0;
            }
        }
    }
}

}
