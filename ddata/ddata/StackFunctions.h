#pragma once
#include <cmath>

namespace Linearize {
#define STACK_UNARY_FUNCTION(Function, Derivative)                                                         \
    template <class E>                                                                                     \
    struct StackETD_##Function : Expr<StackETD_##Function<E>> {                                            \
        explicit StackETD_##Function(const Expr<E>& e) : expr(e.cast()), value(std::Function(expr.f())) {} \
        void calcGradient(Stack& stack, const double& multiplier) const {                                  \
            expr.calcGradient(stack, (Derivative)*multiplier);                                             \
        }                                                                                                  \
        double f() const { return value; }                                                                 \
                                                                                                           \
      private:                                                                                             \
        const E& expr;                                                                                     \
        const double value;                                                                                \
    };                                                                                                     \
    template <class E>                                                                                     \
    auto Function(const Expr<E>& expr) {                                                                   \
        return StackETD_##Function<E>(expr);                                                               \
    }

STACK_UNARY_FUNCTION(sqrt, expr.f() > 0.0 ? 0.5 / std::sqrt(expr.f()) : 0.0)
STACK_UNARY_FUNCTION(abs, expr.f() > 0.0 ? 1.0 : -1.0)
STACK_UNARY_FUNCTION(exp, std::exp(expr.f()))
STACK_UNARY_FUNCTION(log, 1.0 / expr.f())
STACK_UNARY_FUNCTION(tanh, 1.0 - std::pow(std::tanh(expr.f()), 2))
STACK_UNARY_FUNCTION(sin, std::cos(expr.f()))
STACK_UNARY_FUNCTION(cos, -std::sin(expr.f()))
STACK_UNARY_FUNCTION(asin, 1.0 / std::sqrt(1.0 - expr.f() * expr.f()))
STACK_UNARY_FUNCTION(acos, -1.0 / std::sqrt(1.0 - expr.f() * expr.f()))

template <class E>
auto real(const Expr<E>& e) {
    return e.cast().f();
}

template <class Left, class Right>
struct Pow : Expr<Pow<Left, Right>> {
    Pow(const Expr<Left>& left, const Expr<Right>& right)
        : l(left.cast()), r(right.cast()), value(std::pow(l.f(), r.f())) {}
    void calcGradient(Stack& stack, const double& multiplier) const {
        l.calcGradient(stack, r.f() * value / l.f() * multiplier);
        r.calcGradient(stack, std::log(l.f()) * value * multiplier);
    }
    double f() const { return value; }

  private:
    const Left& l;
    const Right& r;
    const double value;
};
template <class Left, class Right>
auto pow(const Expr<Left>& left, const Expr<Right>& right) {
    return Pow<Left, Right>(left, right);
}

template <class E>
struct PowScalarBase : Expr<PowScalarBase<E>> {
    PowScalarBase(double b, const Expr<E>& e) : base(b), exponent(e.cast()), value(std::pow(base, exponent.f())) {}
    void calcGradient(Stack& stack, const double& multiplier) const {
        exponent.calcGradient(stack, value * std::log(base) * multiplier);
    }
    double f() const { return value; }

  private:
    const double base;
    const E& exponent;
    const double value;
};
template <class E>
auto pow(double base, const Expr<E>& exponent) {
    return PowScalarBase<E>(base, exponent);
}

template <class E>
struct PowScalarExponent : Expr<PowScalarExponent<E>> {
    PowScalarExponent(const Expr<E>& b, double e) : base(b.cast()), exponent(e), value(std::pow(base.f(), exponent)) {}
    void calcGradient(Stack& stack, const double& multiplier) const {
        base.calcGradient(stack, exponent * value / base.f() * multiplier);
    }
    double f() const { return value; }

  private:
    const E& base;
    const double exponent;
    const double value;
};
template <class E>
auto pow(const Expr<E>& base, double exponent) {
    return PowScalarExponent<E>(base, exponent);
}

template <class E>
struct PowIntExponent : Expr<PowIntExponent<E>> {
    PowIntExponent(const Expr<E>& b, int e) : base(b.cast()), exponent(e), value(std::pow(base.f(), exponent)) {}
    void calcGradient(Stack& stack, const double& multiplier) const {
        base.calcGradient(stack, exponent * value / base.f() * multiplier);
    }
    double f() const { return value; }

  private:
    const E& base;
    const int exponent;
    const double value;
};

template <class E>
auto pow(const Expr<E>& base, int exponent) {
    return PowIntExponent<E>(base, exponent);
}
}
