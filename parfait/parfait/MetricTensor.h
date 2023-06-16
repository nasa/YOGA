#pragma once
#include <parfait/DenseMatrix.h>
#include <parfait/MetricDecomposition.h>
#include <parfait/Point.h>
#include <cmath>
#include <vector>
#include "MotionMatrix.h"

namespace Parfait {

using Tensor = DenseMatrix<double, 3, 3>;

class MetricTensor {
  public:
    static double edgeLength(const Tensor& M, const Point<double>& edge) {
        Vector<double, 3> e{edge[0], edge[1], edge[2]};
        return std::sqrt(e.dot(M * e));
    }

    static Tensor metricFromEllipse(const Point<double>& axis_1,
                                    const Point<double>& axis_2,
                                    const Point<double>& axis_3) {
        MetricDecomposition::Decomposition decomp;
        decomp.D.clear();
        double h = axis_1.magnitude();
        decomp.D(0, 0) = 1.0 / (h * h);
        h = axis_2.magnitude();
        decomp.D(1, 1) = 1.0 / (h * h);
        h = axis_3.magnitude();
        decomp.D(2, 2) = 1.0 / (h * h);

        auto eigenvector_1 = axis_1;
        eigenvector_1.normalize();
        auto eigenvector_2 = axis_2;
        eigenvector_2.normalize();
        auto eigenvector_3 = axis_3;
        eigenvector_3.normalize();

        for (int i = 0; i < 3; i++) {
            decomp.R(i, 0) = eigenvector_1[i];
            decomp.R(i, 1) = eigenvector_2[i];
            decomp.R(i, 2) = eigenvector_3[i];
        }

        return MetricDecomposition::recompose(decomp);
    }

    static Tensor metricFromEllipse(double h1, double h2, double h3, double alpha, double beta, double gamma) {
        alpha *= M_PI / 180.0;
        beta *= M_PI / 180.0;
        gamma *= M_PI / 180.0;
        h1 = h1 * h1;
        h2 = h2 * h2;
        h3 = h3 * h3;
        double sa = sin(alpha);
        double sb = sin(beta);
        double sg = sin(gamma);
        double ca = cos(alpha);
        double cb = cos(beta);
        double cg = cos(gamma);
        DenseMatrix<double, 3, 3> R{{ca * cb, ca * sb * sg - sa * cg, ca * sb * cg + sa * sg},
                                    {sa * cb, sa * sb * sg + ca * cg, sa * sb * cg - ca * sg},
                                    {-sb, cb * sg, cb * cg}};
        // printf("R:\n");
        // print(R);
        DenseMatrix<double, 3, 3> D{{1.0 / h1, 0, 0}, {0, 1.0 / h2, 0}, {0, 0, 1.0 / h3}};
        // printf("D:\n");
        // print(D);
        return R * D * R.transpose();
    }

    static double logEuclideanDistance(const Tensor& A, const Tensor& B) {
        auto L1 = metricTransform(A, Op::Log);
        auto L2 = metricTransform(B, Op::Log);
        auto diff = L1 - L2;
        auto squared = metricTransform(diff, Op::Square);
        return std::sqrt(squared.trace());
    }

    static Tensor logEuclideanAverage(const std::vector<Tensor>& tensors, const std::vector<double>& weights) {
        Tensor mean;
        mean.clear();
        for (size_t i = 0; i < tensors.size(); i++) {
            try {
                mean = mean + (metricTransform(tensors[i], Op::Log) * weights[i]);
            } catch (...) {
                std::stringstream ss;
                ss << "Mean:\n";
                ss << MetricDecomposition::formatMatrixAsUnitTestFixture(mean);
                ss << "Input tensors:\n";
                // for(auto& tensor:tensors)
                ss << MetricDecomposition::formatMatrixAsUnitTestFixture(tensors[i]);
                ss << "Weights\n";
                for (auto w : weights) ss << w << " ";
                ss << "\n";
                printf("Log euclidean average failed:\n%s\n", ss.str().c_str());
                PARFAIT_THROW("Log euclidean average failed");
            }
        }
        return metricTransform(mean, Op::Exponential);
    }

    static Tensor invert(const Tensor& M) {
        auto d = MetricDecomposition::decompose(M);
        for (int i = 0; i < 3; i++) d.D(i, i) = 1.0 / d.D(i, i);
        return d.R * d.D * d.R.transpose();
    }

    static Tensor rotate(const Tensor& A, const Parfait::MotionMatrix& rotation) {
        DenseMatrix<double, 4, 4> m;
        rotation.getMatrix(m.data());
        Tensor M = DenseMatrix<double, 4, 4>::SubMatrix(0, 0, 3, 3, m);

        auto Mt = M.transpose();
        Tensor B = A * Mt;
        B = M * B;
        return B;
    }

    static Vector<double, 3> extractEigenvector(const MetricDecomposition::Decomposition& decomp, int n) {
        Vector<double, 3> e{0, 0, 0};
        for (int i = 0; i < 3; i++) e[i] = decomp.R(n, i);
        return e;
    }

    static Tensor rayleighFormula(const Tensor& M, const Tensor& N) {
        Tensor v = Tensor::Identity();
        auto decomp = MetricDecomposition::decompose(N);
        decomp.R.transposeInPlace();
        for (int i = 0; i < 3; i++) {
            const auto e = extractEigenvector(decomp, i);
            v(i, i) = e.dot(M * e);
        }
        return v;
    }

    static Tensor intersect(const Tensor& M1, const Tensor& M2) { return intersectAlauzet(M1, M2); }

    static Tensor intersectAlauzet(const Tensor& M1, const Tensor& M2) {
        auto M1_inv = invert(M1);
        auto N = M1_inv * M2;

        auto N_decomp = MetricDecomposition::decompose(N);
        auto P = N_decomp.R;

        auto lambdas = rayleighFormula(M1, N);
        auto mus = rayleighFormula(M2, N);
        Tensor L = Tensor::Identity();
        for (int i = 0; i < 3; i++) L(i, i) = std::max(lambdas(i, i), mus(i, i));
        return P * L * P.transpose();
    }

  private:
    enum Op { Log, Exponential, Root, Square };
    static Tensor metricTransform(const Tensor& m, Op op) {
        auto decomp = MetricDecomposition::decompose(m);
        auto& D = decomp.D;
        auto& R = decomp.R;
        for (int i = 0; i < 3; i++) {
            switch (op) {
                case Op::Log:
                    D(i, i) = std::max(D(i, i), 1e-10);
                    D(i, i) = std::log(D(i, i));
                    break;
                case Op::Exponential:
                    D(i, i) = std::exp(D(i, i));
                    break;
                case Op::Root:
                    D(i, i) = std::sqrt(D(i, i));
                    break;
                case Op::Square:
                    D(i, i) *= D(i, i);
                    break;
            }
            if (!std::isfinite(D(i, i))) PARFAIT_THROW("metricTransofrm generated a non-finite number and failed");
        }
        return R * D * R.transpose();
    }
};

}