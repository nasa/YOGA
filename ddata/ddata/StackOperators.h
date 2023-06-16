#pragma once

#include "Expr.h"

namespace Linearize {

#define STACK_BINARY_OP(Op_Name, op, left_deriv, right_deriv)                                          \
    template <class Left, class Right>                                                                 \
    struct Op_Name : Expr<Op_Name<Left, Right>> {                                                      \
        Op_Name(const Expr<Left>& left, const Expr<Right>& right) : l(left.cast()), r(right.cast()) {} \
        void calcGradient(Stack& stack, const double& multiplier) const {                              \
            l.calcGradient(stack, left_deriv);                                                         \
            r.calcGradient(stack, right_deriv);                                                        \
        }                                                                                              \
        double f() const { return l.f() op r.f(); }                                                    \
                                                                                                       \
      private:                                                                                         \
        const Left& l;                                                                                 \
        const Right& r;                                                                                \
    };                                                                                                 \
    template <class Left, class Right>                                                                 \
    auto operator op(const Expr<Left>& left, const Expr<Right>& right) {                               \
        return Op_Name<Left, Right>(left, right);                                                      \
    }

STACK_BINARY_OP(Add, +, multiplier, multiplier)
STACK_BINARY_OP(Subtract, -, multiplier, -multiplier)
STACK_BINARY_OP(Multiply, *, r.f() * multiplier, l.f() * multiplier)
STACK_BINARY_OP(Divide, /, r.f() * multiplier / (r.f() * r.f()), -multiplier* l.f() / (r.f() * r.f()))

#define STACK_BINARY_SCALAR_OP_CLASS(Op_Name, value, derivative)                                               \
    template <class E>                                                                                         \
    struct Op_Name : Expr<Op_Name<E>> {                                                                        \
        Op_Name(double scalar, const Expr<E>& expr) : scalar(scalar), e(expr.cast()) {}                        \
        void calcGradient(Stack& stack, const double& multiplier) const { e.calcGradient(stack, derivative); } \
        double f() const { return value; }                                                                     \
                                                                                                               \
      private:                                                                                                 \
        const double scalar;                                                                                   \
        const E& e;                                                                                            \
    };

#define STACK_BINARY_SCALAR_OP(Op_Name, op, left_derivative, right_derivative)       \
    STACK_BINARY_SCALAR_OP_CLASS(Left_##Op_Name, e.f() op scalar, left_derivative)   \
    STACK_BINARY_SCALAR_OP_CLASS(Right_##Op_Name, scalar op e.f(), right_derivative) \
    template <typename E>                                                            \
    auto operator op(const Expr<E>& e, const double& scalar) {                       \
        return Left_##Op_Name<E>(scalar, e.cast());                                  \
    }                                                                                \
    template <typename E>                                                            \
    auto operator op(const double& scalar, const Expr<E>& e) {                       \
        return Right_##Op_Name<E>(scalar, e.cast());                                 \
    }

STACK_BINARY_SCALAR_OP(Add, +, multiplier, multiplier)
STACK_BINARY_SCALAR_OP(Subtract, -, multiplier, -multiplier)
STACK_BINARY_SCALAR_OP(Multiply, *, scalar* multiplier, multiplier* scalar)
STACK_BINARY_SCALAR_OP(Divide, /, multiplier / scalar, -scalar* multiplier / (e.f() * e.f()))

template <typename E>
struct Negate : Expr<Negate<E>> {
    Negate(const Expr<E>& expr) : e(expr.cast()) {}
    void calcGradient(Stack& stack, const double& multiplier) const { e.calcGradient(stack, -multiplier); }
    double f() const { return -e.f(); }

  private:
    const E& e;
};
template <class E>
auto operator-(const Expr<E>& e) {
    return Negate<E>(e);
}

#define STACK_COMPARATOR(op)                                        \
    template <class Left, class Right>                              \
    bool operator op(const Expr<Left>& e1, const Expr<Right>& e2) { \
        return e1.cast().f() op e2.cast().f();                      \
    }                                                               \
    template <class E>                                              \
    bool operator op(const double& value, const Expr<E>& e) {       \
        return value op e.cast().f();                               \
    }                                                               \
    template <class E>                                              \
    bool operator op(const Expr<E>& e, const double& value) {       \
        return e.cast().f() op value;                               \
    }

STACK_COMPARATOR(==)
STACK_COMPARATOR(!=)
STACK_COMPARATOR(<)
STACK_COMPARATOR(>)
STACK_COMPARATOR(<=)
STACK_COMPARATOR(>=)

template <typename E>
std::ostream& operator<<(std::ostream& os, const Expr<E>& e) {
    os << e.cast().f();
    return os;
}
}
