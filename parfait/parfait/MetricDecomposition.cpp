#include "MetricDecomposition.h"
#include <string.h>
#include "StringTools.h"
#include <limits>
Parfait::MetricDecomposition::Decomposition Parfait::MetricDecomposition::decompose(
    const Parfait::DenseMatrix<double, 3, 3>& M) {
    Decomposition decomp;
    decomp.D = M;
    forceSymmetry(decomp.D);
    decomp.R = eliminate02WithHouseholderReflection(decomp.D);
    bool succeeded = iterativelyDiagonalize(decomp.R, decomp.D);
    if (not succeeded) throwFailedToDiagonalize(M);
    return decomp;
}
Parfait::DenseMatrix<double, 3, 3> Parfait::MetricDecomposition::recompose(
    const Parfait::MetricDecomposition::Decomposition& d) {
    return d.R * d.D * d.R.transpose();
}
double Parfait::MetricDecomposition::calcTargetError(const Parfait::DenseMatrix<double, 3, 3>& M) {
    return M.norm() * std::numeric_limits<double>::epsilon();
}
double Parfait::MetricDecomposition::frobeniusOfOffDiagonals(const Parfait::DenseMatrix<double, 3, 3>& M) {
    double e = 0.0;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (i != j) e += M(i, j) * M(i, j);
    return e;
}
void Parfait::MetricDecomposition::printMatrixAsUnitTestFixture(const Parfait::DenseMatrix<double, 3, 3>& M) {
    printf("%s\n", formatMatrixAsUnitTestFixture(M).c_str());
}
std::string Parfait::MetricDecomposition::formatMatrixAsUnitTestFixture(const Parfait::DenseMatrix<double, 3, 3>& M) {
    char s[1024];
    sprintf(s, "DenseMatrix<double,3,3> M {\n");
    sprintf(s + strlen(s), "\t{%.15e,%.15e,%.15e},\n", M(0, 0), M(0, 1), M(0, 2));
    sprintf(s + strlen(s), "\t{%.15e,%.15e,%.15e},\n", M(1, 0), M(1, 1), M(1, 2));
    sprintf(s + strlen(s), "\t{%.15e,%.15e,%.15e}", M(2, 0), M(2, 1), M(2, 2));
    sprintf(s + strlen(s), "};");
    std::string fixture(s);
    return Parfait::StringTools::rstrip(fixture) + "\n\n";
}
void Parfait::MetricDecomposition::throwFailedToDiagonalize(const Parfait::DenseMatrix<double, 3, 3>& M) {
    printMatrixAsUnitTestFixture(M);
    throw std::logic_error("Failed to diagonalize matrix.");
}
bool Parfait::MetricDecomposition::iterativelyDiagonalize(Parfait::DenseMatrix<double, 3, 3>& R,
                                                          Parfait::DenseMatrix<double, 3, 3>& M) {
    double target_error = calcTargetError(M);
    for (int i = 0; i < 100; i++) {
        double error = frobeniusOfOffDiagonals(M);
        if (error <= target_error) break;
        if (shouldReduce12Entry(M)) {
            auto G = reduce12WithGivensRotation(M);
            R = R * G;
        } else {
            auto G = reduce01WithGivensRotation(M);
            R = R * G;
        }
    }
    double error = frobeniusOfOffDiagonals(M);
    return error <= target_error;
}
void Parfait::MetricDecomposition::forceSymmetry(Parfait::DenseMatrix<double, 3, 3>& m) {
    m(1, 0) = m(0, 1);
    m(2, 0) = m(0, 2);
    m(2, 1) = m(1, 2);
}
bool Parfait::MetricDecomposition::shouldReduce12Entry(Parfait::DenseMatrix<double, 3, 3>& B) {
    return std::abs(B(1, 2)) <= std::abs(B(0, 1));
}
Parfait::DenseMatrix<double, 3, 3> Parfait::MetricDecomposition::eliminate02WithHouseholderReflection(
    Parfait::DenseMatrix<double, 3, 3>& M) {
    double a12 = M(1, 2);
    double a02 = M(0, 2);
    double target_error = calcTargetError(M);
    if (a02 <= target_error) return DenseMatrix<double, 3, 3>::Identity();
    double denom = std::sqrt(a02 * a02 + a12 * a12);
    double c = a12 / denom;
    double s = -a02 / denom;
    DenseMatrix<double, 3, 3> H{{c, s, 0}, {s, -c, 0}, {0, 0, 1}};
    DenseMatrix<double, 3, 3> result = H.transpose() * M * H;
    M = result;
    return H;
}
Parfait::DenseMatrix<double, 3, 3> Parfait::MetricDecomposition::reduce01WithGivensRotation(
    Parfait::DenseMatrix<double, 3, 3>& B) {
    double b12 = B(1, 2);
    double b11 = B(1, 1);
    double b22 = B(2, 2);
    double denom = sqrt((b22 - b11) * (b22 - b11) + 4.0 * b12 * b12);
    double sigma = (b22 - b11) < 0 ? -1 : 1;
    double cos2theta = -std::abs(b22 - b11) / denom;
    double sin2theta = -2 * sigma * b12 / denom;
    double sin_theta = sqrt(.5 * (1 - cos2theta));
    double cos_theta = .5 * sin2theta / sin_theta;
    double c = cos_theta;
    double s = sin_theta;
    DenseMatrix<double, 3, 3> G{{0, 1, 0}, {c, 0, s}, {-s, 0, c}};
    DenseMatrix<double, 3, 3> result = G.transpose() * B * G;
    B = result;
    return G;
}
Parfait::DenseMatrix<double, 3, 3> Parfait::MetricDecomposition::reduce12WithGivensRotation(
    Parfait::DenseMatrix<double, 3, 3>& B) {
    double b00 = B(0, 0);
    double b11 = B(1, 1);
    double b01 = B(0, 1);
    double denom = sqrt((b00 - b11) * (b00 - b11) + 4.0 * b01 * b01);
    double sigma = (b00 - b11) < 0 ? -1 : 1;
    double cos2theta = -std::abs(b00 - b11) / denom;
    double sin2theta = -2 * sigma * b01 / denom;
    double sin_theta = sqrt(.5 * (1 - cos2theta));
    double cos_theta = .5 * sin2theta / sin_theta;
    double c = cos_theta;
    double s = sin_theta;
    DenseMatrix<double, 3, 3> G{{c, 0, -s}, {s, 0, c}, {0, 1, 0}};
    DenseMatrix<double, 3, 3> result = G.transpose() * B * G;
    B = result;
    return G;
}
