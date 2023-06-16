#pragma once
#include <array>

constexpr int NumEqns() { return 8; }

template <typename T>
using Array = std::array<T, NumEqns()>;

template <typename T>
T func(const T& a, const T& b) {
    return a + b + sin(a) + sqrt(a);
}

constexpr double R() { return 287.0; }
constexpr double cv() { return 1234.0; }

template <typename T>
inline T calcEnergy(const Array<T>& q) {
    return q[0] * (cv() * q[4] + 0.5 * (q[1] * q[1] + q[2] * q[2] + q[3] * q[3]));
}

template <typename T>
inline T calcPressure(const Array<T>& q) {
    constexpr double Rinv = 1.0 / R();
    return q[0] / q[4] * Rinv;
}

template <typename T>
void calcFlux(Array<T>& flux, const Array<T>& q, const std::array<double, 3>& n) {
    const T u_n = q[1] * n[0] + q[2] * n[1] + q[3] * n[2];
    const T p = calcPressure(q);

    flux[0] += q[0] * u_n;
    flux[1] += q[0] * q[1] * u_n + p * n[0];
    flux[2] += q[0] * q[2] * u_n + p * n[1];
    flux[3] += q[0] * q[3] * u_n + p * n[2];
    flux[4] += (calcEnergy(q) + p) * u_n;
}
void calcJacobian(Array<Array<double>>& jacobian, const Array<double>& q, const std::array<double, 3>& n) {
    double u_n = q[1] * n[0] + q[2] * n[1] + q[3] * n[2];
    double p = calcPressure(q);

    double dp_drho = 1.0 / (R() * q[4]);
    double dp_dT = -p / q[4];

    auto e = calcEnergy(q);
    auto enthalpy = e + p;

    //     q[0] * (cv() * q[4] + 0.5 * (q[1] * q[1] + q[2] * q[2] + q[3] * q[3]));
    double de_rho = e / q[0];
    double de_u = q[0] * q[1];
    double de_v = q[0] * q[2];
    double de_w = q[0] * q[3];
    double de_T = q[0] * cv();

    //     flux[0] = q[0] * u_n;
    jacobian[0][0] += u_n;
    jacobian[0][1] += q[0] * n[0];
    jacobian[0][2] += q[0] * n[1];
    jacobian[0][3] += q[0] * n[2];

    //     flux[1] = q[0] * q[1] * u_n + p * n[0];
    jacobian[1][0] += q[1] * u_n + dp_drho * n[0];
    jacobian[1][1] += q[0] * (u_n + q[1] * n[0]);
    jacobian[1][2] += q[0] * q[1] * n[0];
    jacobian[1][3] += q[0] * q[1] * n[0];
    jacobian[1][4] += dp_dT * n[0];

    //    flux[2] = q[0] * q[2] * u_n + p * n[1];
    jacobian[2][0] += q[2] * u_n + dp_drho * n[1];
    jacobian[2][1] += q[0] * q[2] * n[1];
    jacobian[2][2] += q[0] * (u_n + q[2] * n[1]);
    jacobian[2][3] += q[0] * q[2] * n[1];
    jacobian[2][4] += dp_dT * n[1];

    //    flux[3] = q[0] * q[3] * u_n + p * n[2];
    jacobian[3][0] += q[3] * u_n + dp_drho * n[2];
    jacobian[3][1] += q[0] * q[3] * n[2];
    jacobian[3][2] += q[0] * q[3] * n[2];
    jacobian[3][3] += q[0] * (u_n + q[3] * n[2]);
    jacobian[3][4] += dp_dT * n[2];

    //     flux[4] = (calcEnergy(q) + p) * u_n;
    jacobian[4][0] += (de_rho + dp_drho) * u_n;
    jacobian[4][1] += de_u * u_n + enthalpy * n[0];
    jacobian[4][2] += de_v * u_n + enthalpy * n[1];
    jacobian[4][3] += de_w * u_n + enthalpy * n[2];
    jacobian[4][4] += (de_T + dp_dT) * u_n;
}
