#pragma once
#include <array>
#include <iostream>
#include <numeric>
#include <parfait/Throw.h>
#include <parfait/DenseMatrix.h>

namespace YOGA {

class Lagrange{
      public:
        template<typename T>
        inline static Parfait::Point<T> evaluateBasis(T r,T s,T t,
                                                    T* coeff,
                                                    Parfait::Point<T>* points,
                                                    int n) {
            Parfait::Point<T> dr = {0, 0, 0};
            for (int i = 0; i < n; i++) {
                for(int j=0;j<3;j++)
                    dr[j] += coeff[i] * points[i][j];
            }
            return dr;
        }

        template<typename T>
        static double norm(const Parfait::Vector<T,3>& vec){
            double r=0;
            for(int i=0;i<3;i++) {
                double v = std::abs(Linearize::real(vec[i]));
                r = std::max(r,v);
            }
            return r;
        }

        template<typename T1,typename T2>
        static Parfait::Point<T2> scale(T1 a,const Parfait::Point<T2>& b){
            return {a*b[0], a*b[1], a*b[2]};
        }

        template<typename T,typename F,typename F2>
        inline static Parfait::Point<T> calcComputationalCoordinates(const Parfait::Point<T>& p,
                Parfait::Point<T>* vertices,
                int n,
                F dbdrCoeff,
                F dbdsCoeff,
                F dbdtCoeff,
                F2 evaluate
                ) {
            typedef Parfait::Vector<T,3> Vector;
            typedef Parfait::DenseMatrix<T,3,3> Matrix;
            int max_iterations = 1000;
            double converge_tolerance = 1.0e-12;
            Vector rst = {.5, .5, .5};
            Vector residual, update;
            for (int i = 0; i < max_iterations; i++) {
                const Vector& rst1 = rst;
                const Parfait::Point<T>& p1 = p;
                auto pAtGreek = evaluate(rst1[0], rst1[1], rst1[2]);
                residual[0] = pAtGreek[0] - p1[0];
                residual[1] = pAtGreek[1] - p1[1];
                residual[2] = pAtGreek[2] - p1[2];
                if(norm(residual) < converge_tolerance) {
                    break;
                }
                auto coeff = dbdrCoeff(rst[0], rst[1], rst[2]);
                auto dBdr = evaluateBasis(rst[0], rst[1], rst[2], coeff.data(), vertices, n);
                coeff = dbdsCoeff(rst[0], rst[1], rst[2]);
                auto dBds = evaluateBasis(rst[0], rst[1], rst[2], coeff.data(), vertices, n);
                coeff = dbdtCoeff(rst[0], rst[1], rst[2]);
                auto dBdt = evaluateBasis(rst[0], rst[1], rst[2], coeff.data(), vertices, n);
                Matrix jac;
                jac(0, 0) = Linearize::real(dBdr[0]);
                jac(0, 1) = Linearize::real(dBds[0]);
                jac(0, 2) = Linearize::real(dBdt[0]);

                jac(1, 0) = Linearize::real(dBdr[1]);
                jac(1, 1) = Linearize::real(dBds[1]);
                jac(1, 2) = Linearize::real(dBdt[1]);

                jac(2, 0) = Linearize::real(dBdr[2]);
                jac(2, 1) = Linearize::real(dBds[2]);
                jac(2, 2) = Linearize::real(dBdt[2]);
                update = Parfait::GaussJordan::solve(jac,residual);
                rst -= update;
            }
            return {rst[0], rst[1], rst[2]};
        }

    };

template<typename T>
class LagrangeTet{
  public:
    LagrangeTet(const std::array<Parfait::Point<T>, 4>& pts) { points = pts; }
    inline Parfait::Point<T> calcComputationalCoordinates(const Parfait::Point<T>& p) {
        auto eval = [&](T r,T s,T t){
            return evaluate(r,s,t);
        };
        return Lagrange::calcComputationalCoordinates(p,points.data(),4,dbdrCoeff,dbdsCoeff,dbdtCoeff,eval);
    }

    inline std::array<T, 4> evaluateBasis(T r,T s,T t) const {
        return {1.0 - r - s - t, r, s, t};
    }

    inline Parfait::Point<T> evaluate(T r, T s, T t) const {
        auto basis = evaluateBasis(r, s, t);
        Parfait::Point<T> p = {0, 0, 0};
        for (int i = 0; i < 4; i++) {
            for(int j=0;j<3;j++)
                p[j] += basis[i] * points[i][j];
        }
        return p;
    }

  private:
    std::array<Parfait::Point<T>,4> points;
    static inline std::array<T, 4> dbdrCoeff(T r, T s, T t) { return {-1, 1, 0, 0}; }
    static inline std::array<T, 4> dbdsCoeff(T r, T s, T t) { return {-1, 0, 1, 0}; }
    static inline std::array<T, 4> dbdtCoeff(T r, T s, T t) { return {-1, 0, 0, 1}; }
};

template<typename T>
class LagrangePyramid{
  public:
    LagrangePyramid(const std::array<Parfait::Point<T>, 5>& pts) { points = pts; }
    inline Parfait::Point<T> calcComputationalCoordinates(const Parfait::Point<T>& p) {
        auto eval = [&](T r,T s, T t){
            return evaluate(r,s,t);
        };
        return Lagrange::calcComputationalCoordinates(p,points.data(),5,dbdrCoeff,dbdsCoeff,dbdtCoeff,eval);
    }

    inline Parfait::Point<T> evaluate(T r, T s, T t) const {
        auto basis = evaluateBasis(r, s, t);
        Parfait::Point<T> p = {0, 0, 0};
        for (int i = 0; i < 5; i++)
                p += Lagrange::scale(basis[i], points[i]);
        return p;
    }

    inline std::array<T, 5> evaluateBasis(T r, T s, T t) const {
        return {
            -1. * (r - 1.) * (s - 1.) * (t - 1.),
            r * (s - 1.) * (t - 1.), -1. * r * s * (t - 1.),
            s * (r - 1.) * (t - 1.),
            t};
    }

  private:
    std::array<Parfait::Point<T>,5> points;
    static inline std::array<T, 5> dbdrCoeff(T r, T s, T t){
        return {-(s - 1.) * (t - 1.), (s - 1.) * (t - 1.), -s * (t - 1.), s * (t - 1.), 0};
    }
    static inline std::array<T, 5> dbdsCoeff(T r, T s, T t){
        return {-(r - 1.) * (t - 1.), (s - 1.) * (t - 1.), -r * (t - 1.), (r - 1.) * (t - 1.), 0};
    }
    static inline std::array<T, 5> dbdtCoeff(T r, T s, T t){
        return {-(r - 1.) * (s - 1.), r * (s - 1.), r * s, s * (r - 1.), 1.};
    }
};

template<typename T>
class LagrangePrism{
  public:
    LagrangePrism(const std::array<Parfait::Point<T>, 6>& pts) { points = pts; }
    inline Parfait::Point<T> calcComputationalCoordinates(const Parfait::Point<T>& p) {
        auto eval = [&](T r,T s, T t){
            return evaluate(r,s,t);
        };
        return Lagrange::calcComputationalCoordinates(p,points.data(),6,dbdrCoeff,dbdsCoeff,dbdtCoeff,eval);
    }

    inline Parfait::Point<T> evaluate(T r, T s, T t) const {
        auto basis = evaluateBasis(r, s, t);
        Parfait::Point<T> p = {0, 0, 0};
        for (int i = 0; i < 6; i++) {
            p += Lagrange::scale(basis[i], points[i]);
        }
        return p;
    }

    inline std::array<T, 6> evaluateBasis(T r, T s, T t) const {
        return {(r + s - 1.) * (t - 1.), r - r * t, s - s * t, -(r + s - 1.) * t, r * t, s * t};
    }

  private:
    std::array<Parfait::Point<T>,6> points;
    static inline std::array<T, 6> dbdrCoeff(T r, T s, T t){
        return {t - 1., 1. - t, 0, t - 1., t, 0};
    }
    static inline std::array<T, 6> dbdsCoeff(T r, T s, T t){
        return {t - 1., 0, 1. - t, t - 1., 0, t};
    }
    static inline std::array<T, 6> dbdtCoeff(T r, T s, T t){
        return {r + s - 1., -r, -s, 1. - r - s, r, s};
    }
};

template<typename T>
class LagrangeHex {
  public:
    LagrangeHex(const std::array<Parfait::Point<T>, 8>& pts) { points = pts; }
    inline Parfait::Point<T> calcComputationalCoordinates(const Parfait::Point<T>& p) {
        auto eval = [&](T r,T s, T t){
          return evaluate(r,s,t);
        };
        return Lagrange::calcComputationalCoordinates(p,points.data(),8,dbdrCoeff,dbdsCoeff,dbdtCoeff,eval);
    }

    inline Parfait::Point<T> evaluate(T r, T s, T t) const {
        auto basis = evaluateBasis(r, s, t);
        Parfait::Point<T> p = {0, 0, 0};
        for (size_t i = 0; i < points.size(); i++) {
            p += Lagrange::scale(basis[i], points[i]);
        }
        return p;
    }

    inline std::array<T, 8> evaluateBasis(T r, T s, T t) const {
        return {-(r - 1.) * (s - 1.) * (t - 1.),
                r * (s - 1.) * (t - 1.),
                -r * s * (t - 1.),
                s * (r - 1.) * (t - 1.),
                t * (r - 1.) * (s - 1.),
                -r * t * (s - 1.),
                r * s * t,
                -s * t * (r - 1.)};
    }

  private:
    std::array<Parfait::Point<T>,8> points;
    static inline std::array<T, 8> dbdrCoeff(T r, T s, T t) {
        return {s + t - s * t - 1., (s - 1.) * (t - 1.), s - s * t, s * (t - 1.), t * (s - 1.), t - s * t, s * t, -s * t};
    }
    static inline std::array<T, 8> dbdsCoeff(T r, T s, T t) {
        return {r + t - r * t - 1., r * (t - 1.), r - r * t, (r - 1.) * (t - 1.), t * (r - 1.), -r * t, r * t, t - r * t};
    }
    static inline std::array<T, 8> dbdtCoeff(T r, T s, T t) {
        return {r + s - r * s - 1., r * (s - 1.), -r * s, s * (r - 1.), (r - 1.) * (s - 1.), r - r * s, r * s, s - r * s};
    }
};
}
