// This file was adapted from:
//
//  https://gitlab.larc.nasa.gov/fun3d-developers/surreal/-/blob/master/test/euler_flux_1d_surreal_test.cpp
//
//  and is intended as an acceptance test for comparison of new reverse-mode AD with existing Surreal implementations

/* run script
g++ euler_flux_1d_surreal_test.cpp -Ofast -march=native -std=c++14 -I../src/ -I../src/surreal -o
euler_flux_1d_surreal_test echo "g++ euler_flux_1d_surreal_test.cpp -Ofast -mcpu=native -std=c++14 -I../src/
-I../src/surreal -o euler_flux_1d_surreal_test" > screen.out lscpu >> screen.out g++ --version >> screen.out
./euler_flux_1d_surreal_test >> screen.out
*/

/* screen.out
g++ euler_flux_1d_surreal_test.cpp -Ofast -mcpu=native -std=c++14 -I../src/ -I../src/surreal -o
euler_flux_1d_surreal_test Architecture:          ppc64le Byte Order:            Little Endian CPU(s): 160 On-line
CPU(s) list:   0-159 Thread(s) per core:    4 Core(s) per socket:    20 Socket(s):             2 NUMA node(s): 6 Model:
2.2 (pvr 004e 1202) Model name:            POWER9, altivec supported CPU max MHz:           3800.0000 CPU min MHz:
2300.0000 L1d cache:             32K L1i cache:             32K L2 cache:              512K L3 cache: 10240K NUMA node0
CPU(s):     0-79 NUMA node8 CPU(s):     80-159 NUMA node252 CPU(s): NUMA node253 CPU(s): NUMA node254 CPU(s): NUMA
node255 CPU(s): g++ (GCC) 7.3.0 Copyright (C) 2017 Free Software Foundation, Inc. This is free software; see the source
for copying conditions.  There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

Primitive Variables and Derivatives
i,q,dq/dQ
+1 +1.000000e+00 : +1.000000e+00 +0.000000e+00 +0.000000e+00
+2 +1.000000e+00 : -1.000000e+00 +1.000000e+00 +0.000000e+00
+3 +2.000000e-01 : +2.000000e-01 -4.000000e-01 +4.000000e-01
+4 +6.968968e-04 : +0.000000e+00 -1.393794e-03 +1.393794e-03
Flux and Jacobian Computed with Conservative Variables with surreal
i,F,dF/dQ
+1 +1.000000e+00 : +0.000000e+00 +1.000000e+00 +0.000000e+00
+2 +1.200000e+00 : -8.000000e-01 +1.600000e+00 +4.000000e-01
+3 +1.200000e+00 : -1.000000e+00 +8.000000e-01 +1.400000e+00
Flux and Jacobian Computed with Primitive Variables with surreal
i,F,dF/dQ
+1 +1.000000e+00 : +0.000000e+00 +1.000000e+00 +0.000000e+00
+2 +1.200000e+00 : -8.000000e-01 +1.600000e+00 +4.000000e-01
+3 +1.200000e+00 : -1.000000e+00 +8.000000e-01 +1.400000e+00
Flux and Jacobian Computed by Hand
i,F,dF/dQ
+1 +1.000000e+00 : +0.000000e+00 +1.000000e+00 +0.000000e+00
+2 +1.200000e+00 : -8.000000e-01 +1.600000e+00 +4.000000e-01
+3 +1.200000e+00 : -1.000000e+00 +8.000000e-01 +1.400000e+00
time hand [s]                    = +2.690727e-03
time surreal conservative [s]    = +2.130826e-02
time surreal primitive [s]       = +1.519431e-02
time surreal conservative / hand = +7.919145e+00
time surreal primitive / hand    = +5.646916e+00
*/

#include <iostream>
#include <cmath>
#include <surreal/surreal.h>
#include "ddata/ETD.h"
#include "ddata/ETDStack.h"
#include <timer.h>

#define LOOP(i, N) for (int i = 0; i < N; ++i)

/* Type definition for floating-point numbers */
using real = double;

#define dpa alignas(SURREAL_ALIGN) real

/* Dimension */
constexpr int ndim = 1;
/* Number of Conservative Variables */
constexpr int nQ = ndim + 2;
/* Number of Primitive Variables */
constexpr int nq = ndim + 3;

/* Math Constants */

constexpr real ZERO = 0.0;
constexpr real ONE = 1.0;
constexpr real TWO = 2.0;
constexpr real THREE = 3.0;
constexpr real FOUR = 4.0;
constexpr real FIVE = 5.0;
constexpr real SIX = 6.0;
constexpr real SEVEN = 7.0;
constexpr real EIGHT = 8.0;
constexpr real HALF = 0.5;
constexpr real FOURTH = 0.25;
constexpr real ONETHIRD = ONE / THREE;
constexpr real ONESIXTH = ONE / SIX;
constexpr real TWOTHIRDS = TWO / THREE;
constexpr real THREEHALVES = 1.5;
constexpr real THREETENTHS = 0.3;
constexpr real ONEEIGHTY = 180.0;
constexpr real PI = M_PI;

/* General Index Constants */

/* Density Index */
constexpr int DENS = 0;
/* X Velocity Index */
constexpr int VELX = 1;
/* Y Velocity Index */
constexpr int VELY = 2;
/* Z Velocity Index */
constexpr int VELZ = 3;
/* Pressure Index */
constexpr int PRES = ndim + 1;
/* Temperature Index */
constexpr int TEMP = ndim + 2;
/* X Momentum Index */
constexpr int MOMX = 1;
/* Y Momentum Index */
constexpr int MOMY = 2;
/* Y Momentum Index */
constexpr int MOMZ = 3;
/* Energy Index */
constexpr int ENER = ndim + 1;
/* X Index */
constexpr int X = 0;
/* Y Index */
constexpr int Y = 1;
/* Y Index */
constexpr int Z = 2;

/* Gas Constants */

/* Universal Gas Constant [J/(kmol K)] */
constexpr real RU = 8314.0;
/* Molecular Weight [g/mol] */
constexpr real MW = 28.97;
/* Gas Constants [J/(kg K)] */
constexpr real R = RU / MW;
/* Specific Heat Ratio */
constexpr real GAM = 1.40;
/* One Minus Specific Heat Ratio */
constexpr real GM1 = (GAM - ONE);
/* Specific Heat at Constant Volume [J/(kg K)] */
constexpr real CV = R / GM1;
/* Specific Heat at Constant Pressure [J/(kg K)] */
constexpr real CP = GAM * CV;
/* Specific Heat at Constant Volume [J/(kg K)] */
constexpr real ICV = GM1 / R;

#define REPEATS 5

template <typename T, size_t N>
inline void calcFlux(std::array<T, N>& flux, const std::array<T, N>& Q) {
    flux[DENS] = Q[MOMX];
    flux[MOMX] = GM1 * Q[ENER] + HALF * (THREE - GAM) * Q[MOMX] * Q[MOMX] / Q[DENS];
    flux[ENER] = GAM * Q[ENER] * Q[MOMX] / Q[DENS] + HALF * GM1 * ((Q[MOMX] * Q[MOMX] * Q[MOMX]) / (Q[DENS] * Q[DENS]));
}

int main() {
    typedef Surreal2<nQ, nQ, real> dT1D;
    typedef Surreal2<nq, nQ, real> dT1DP;
    dpa Q_mem[dT1D::size]{0.0};
    dpa flux_mem[dT1D::size]{0.0};
    dpa q_mem[dT1DP::size]{0.0};
    dT1D Q(Q_mem), flux(flux_mem);
    dT1DP q(q_mem);

    std::array<Linearize::ETD<nQ>, nQ> Q2{};
    std::array<Linearize::ETD<nQ>, nQ> flux2{};

    std::array<Linearize::Var, nQ> Q3{};
    std::array<Linearize::Var, nQ> flux3{};

    Timer tic;
    volatile int test = 5;
    int rep = 1000000;
    double time_hand = -ONE;
    double time_surreal_conservative = std::numeric_limits<double>::max();
    double time_surreal_primitive = std::numeric_limits<double>::max();
    double time_etd_conservative = std::numeric_limits<double>::max();
    double time_stacketd_conservative = std::numeric_limits<double>::max();

    /* Print Formatting */
    std::cout << std::scientific << std::showpos;

    /* Initialization */
    Q(DENS) = ONE; /* rho */
    Q(MOMX) = ONE; /* rho*u */
    Q(ENER) = ONE; /* rho*(CV*T+0.5*(u^2)) = rho*E */

    Q(DENS).deriv(DENS) = ONE;
    Q(MOMX).deriv(MOMX) = ONE;
    Q(ENER).deriv(ENER) = ONE;

    for (auto eqn : {DENS, MOMX, ENER}) Q2[eqn] = Linearize::ETD<nQ>(ONE, eqn);

    q(DENS) = Q(DENS);
    q(VELX) = Q(MOMX) / Q(DENS);
    q(TEMP) = (Q(ENER) / Q(DENS) - HALF * (spow(q(VELX), TWO))) * ICV;
    q(PRES) = q(DENS) * R * q(TEMP);

    std::cout << "Primitive Variables and Derivatives" << std::endl;
    std::cout << "i,q,dq/dQ" << std::endl;
    LOOP(i, nq) { std::cout << i + 1 << " " << q(i) << std::endl; }

    std::cout << "Flux and Jacobian Computed with Conservative Variables with surreal" << std::endl;
    for (int repeat = 0; repeat < REPEATS; ++repeat) {
        tic.reset();
        LOOP(r, rep) {
            flux(DENS) = Q(MOMX);
            flux(MOMX) = GM1 * Q(ENER) + HALF * (THREE - GAM) * Q(MOMX) * Q(MOMX) / Q(DENS);
            // flux(ENER) = GAM*Q(ENER)*Q(MOMX)/Q(DENS)+HALF*(ONE-GAM)*(spow(Q(MOMX),THREE)/spow(Q(DENS),TWO));
            flux(ENER) =
                GAM * Q(ENER) * Q(MOMX) / Q(DENS) + HALF * GM1 * ((Q(MOMX) * Q(MOMX) * Q(MOMX)) / (Q(DENS) * Q(DENS)));

            if (test < 0) std::cout << "prevent loop from being optimized away" << std::endl;
        }
        time_surreal_conservative = std::min(tic.elapsed(), time_etd_conservative);
    }
    std::cout << "i,F,dF/dQ" << std::endl;
    LOOP(i, nQ) { std::cout << i + 1 << " " << flux(i) << std::endl; }

    std::cout << "Flux and Jacobian Computed with Conservative Variables with ETD" << std::endl;
    for (int repeat = 0; repeat < REPEATS; ++repeat) {
        tic.reset();
        LOOP(r, rep) {
            calcFlux(flux2, Q2);
            if (test < 0) std::cout << "prevent loop from being optimized away" << std::endl;
        }
        time_etd_conservative = std::min(time_etd_conservative, tic.elapsed());
    }
    std::cout << "i,F,dF/dQ" << std::endl;
    LOOP(i, nQ) {
        std::cout << i + 1 << " " << flux2[i] << " : " << flux2[i].dx(0) << " " << flux2[i].dx(1) << " "
                  << flux2[i].dx(2) << std::endl;
    }

    std::cout << "Flux and Jacobian Computed with Conservative Variables with StackETD" << std::endl;
    std::array<int, nQ> indep{}, dep{};
    for (int eqn = 0; eqn < nQ; ++eqn) {
        indep[eqn] = Q3[eqn].gradient_index;
        dep[eqn] = flux3[eqn].gradient_index;
    }
    std::array<double, nQ * nQ> jacobian{};
    auto setJacobian = [&](int row, int col) -> double& { return jacobian[row * nQ + col]; };
    for (int repeat = 0; repeat < REPEATS; ++repeat) {
        tic.reset();
        LOOP(r, rep) {
            ACTIVE_STACK.beginRecording();
            calcFlux(flux3, Q3);
            ACTIVE_STACK.calcJacobian(indep, dep, setJacobian);
            if (test < 0) std::cout << "prevent loop from being optimized away" << std::endl;
        }
        time_stacketd_conservative = std::min(time_stacketd_conservative, tic.elapsed());
    }

    printf("grad: %lu operator: %lu statement: %lu\n",
           ACTIVE_STACK.gradients.size(),
           ACTIVE_STACK.operations.size(),
           ACTIVE_STACK.statements.size());

    std::cout << "Flux and Jacobian Computed with Primitive Variables with surreal" << std::endl;
    for (int repeat = 0; repeat < REPEATS; ++repeat) {
        tic.reset();
        LOOP(r, rep) {
            flux(DENS) = q(DENS) * q(VELX);
            flux(MOMX) = q(DENS) * q(VELX) * q(VELX) + q(PRES);
            // flux(ENER) = q(DENS)*(CV*q(TEMP)+q(PRES)/q(DENS)+HALF*spow(q(VELX),TWO))*q(VELX);
            flux(ENER) = q(DENS) * (CV * q(TEMP) + q(PRES) / q(DENS) + HALF * q(VELX) * q(VELX)) * q(VELX);

            if (test < 0) std::cout << "prevent loop from being optimized away" << std::endl;
        }
        time_surreal_primitive = std::min(time_surreal_primitive, tic.elapsed());
    }
    std::cout << "i,F,dF/dQ" << std::endl;
    LOOP(i, nQ) { std::cout << i + 1 << " " << flux(i) << std::endl; }

    std::cout << "Flux and Jacobian Computed by Hand" << std::endl;
    real dfdq[nQ * nQ];
#define DFDQ(i, j) dfdq[i + j * nQ]
    real qh[nQ];
    LOOP(i, nQ) { qh[i] = ONE; }
    real fh[nQ];
    for (int repeat = 0; repeat < REPEATS; ++repeat) {
        tic.reset();
        LOOP(r, rep) {
            fh[DENS] = qh[MOMX];
            fh[MOMX] = GM1 * qh[ENER] + HALF * (THREE - GAM) * qh[MOMX] * qh[MOMX] / qh[DENS];
            fh[ENER] =
                GAM * qh[ENER] * qh[MOMX] / qh[DENS] + HALF * (ONE - GAM) * pow(qh[MOMX], THREE) / pow(qh[DENS], TWO);
            DFDQ(DENS, DENS) = ZERO;
            DFDQ(DENS, MOMX) = ONE;
            DFDQ(DENS, ENER) = ZERO;
            DFDQ(MOMX, DENS) = -HALF * (THREE - GAM) * pow(qh[MOMX] / qh[DENS], TWO);
            DFDQ(MOMX, MOMX) = (THREE - GAM) * qh[MOMX] / qh[DENS];
            DFDQ(MOMX, ENER) = GM1;
            DFDQ(ENER, DENS) = -GAM * qh[ENER] * qh[MOMX] / qh[DENS] / qh[DENS] + (GM1)*pow(qh[MOMX] / qh[DENS], THREE);
            DFDQ(ENER, MOMX) = GAM * qh[ENER] / qh[DENS] + THREEHALVES * (ONE - GAM) * pow(qh[MOMX] / qh[DENS], TWO);
            DFDQ(ENER, ENER) = GAM * qh[MOMX] / qh[DENS];

            if (test < 0) std::cout << "prevent loop from being optimized away" << std::endl;
        }
        time_hand = tic.elapsed();
    }

    std::cout << "i,F,dF/dQ" << std::endl;
    LOOP(i, nQ) {
        std::cout << i + 1 << " " << fh[i] << " : " << DFDQ(i, 0) << " " << DFDQ(i, 1) << " " << DFDQ(i, 2)
                  << std::endl;
    }

    std::cout << "time hand [s]                     = " << time_hand << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl;
    std::cout << "time surreal conservative [s]     = " << time_surreal_conservative << std::endl;
    std::cout << "time ETD conservative [s]         = " << time_etd_conservative << std::endl;
    std::cout << "time StackETD conservative [s]    = " << time_stacketd_conservative << std::endl;
    std::cout << "time surreal primitive [s]        = " << time_surreal_primitive << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl;
    printf("time surreal conservative / hand  = %.3f\n", time_surreal_conservative / time_hand);
    printf("time ETD conservative / hand      = %.3f\n", time_etd_conservative / time_hand);
    printf("time StackETD conservative / hand = %.3f\n", time_stacketd_conservative / time_hand);
    printf("time surreal primitive / hand     = %.3f\n", time_surreal_primitive / time_hand);
}
