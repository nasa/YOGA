#pragma once

#include "ETDExpression.h"
#include <ostream>

namespace Linearize {
#define ETD_BINARY_OP(op, Op_Name, derivative)                                                         \
    template <typename E1, typename E2>                                                                \
    class Op_Name : public ETDExpression<Op_Name<E1, E2>> {                                            \
      public:                                                                                          \
        DDATA_DEVICE_FUNCTION Op_Name(const E1& e_1, const E2& e_2) : e1(e_1), e2(e_2) {               \
            ETD_TYPES_COMPATIBLE(E1, E2);                                                              \
        }                                                                                              \
        ETD_NO_COPY_CONSTRUCTOR(Op_Name)                                                               \
        using value_type = typename E1::value_type;                                                    \
        using derivative_type = typename E1::derivative_type;                                          \
        const E1& e1;                                                                                  \
        const E2& e2;                                                                                  \
        DDATA_DEVICE_FUNCTION value_type f() const { return e1.f() op e2.f(); }                        \
        DDATA_DEVICE_FUNCTION derivative_type dx(int i) const { return derivative; }                   \
        DDATA_DEVICE_FUNCTION int size() const { return std::max(e1.size(), e2.size()); }              \
    };                                                                                                 \
    template <typename E1, typename E2>                                                                \
    DDATA_DEVICE_FUNCTION auto operator op(const ETDExpression<E1>& e1, const ETDExpression<E2>& e2) { \
        return Op_Name<E1, E2>(e1.cast(), e2.cast());                                                  \
    }

ETD_BINARY_OP(+, ETDSum, e1.dx(i) + e2.dx(i))
ETD_BINARY_OP(-, ETDDifference, e1.dx(i) - e2.dx(i))
ETD_BINARY_OP(*, ETDMultiplication, e1.dx(i) * e2.f() + e1.f() * e2.dx(i))
ETD_BINARY_OP(/, ETDDivision, (e2.f() * e1.dx(i) - e1.f() * e2.dx(i)) / (e2.f() * e2.f()))

#define ETD_SCALAR_OP_EXPRESSION(Op_Name, value, derivative)                                   \
    template <typename E>                                                                      \
    class Op_Name : public ETDExpression<Op_Name<E>> {                                         \
      public:                                                                                  \
        using value_type = typename E::value_type;                                             \
        using derivative_type = typename E::derivative_type;                                   \
        DDATA_DEVICE_FUNCTION Op_Name(value_type scalar, const E& e) : scalar(scalar), e(e) {} \
        ETD_NO_COPY_CONSTRUCTOR(Op_Name)                                                       \
        const value_type scalar;                                                               \
        const E& e;                                                                            \
        DDATA_DEVICE_FUNCTION value_type f() const { return value; }                           \
        DDATA_DEVICE_FUNCTION derivative_type dx(int i) const { return derivative; }           \
        DDATA_DEVICE_FUNCTION int size() const { return e.size(); }                            \
    };

#define ETD_SCALAR_OP(op, Op_Name, left_deriv, right_deriv)                                            \
    ETD_SCALAR_OP_EXPRESSION(Left_##Op_Name, e.f() op scalar, left_deriv)                              \
    ETD_SCALAR_OP_EXPRESSION(Right_##Op_Name, scalar op e.f(), right_deriv)                            \
    template <typename E>                                                                              \
    DDATA_DEVICE_FUNCTION auto operator op(const ETDExpression<E>& e, typename E::value_type scalar) { \
        return Left_##Op_Name<E>(scalar, e.cast());                                                    \
    }                                                                                                  \
    template <typename E>                                                                              \
    DDATA_DEVICE_FUNCTION auto operator op(typename E::value_type scalar, const ETDExpression<E>& e) { \
        return Right_##Op_Name<E>(scalar, e.cast());                                                   \
    }

ETD_SCALAR_OP(+, ETDSum, e.dx(i), e.dx(i))
ETD_SCALAR_OP(-, ETDDifference, e.dx(i), -e.dx(i))
ETD_SCALAR_OP(*, ETDMultiplication, scalar* e.dx(i), scalar* e.dx(i))
ETD_SCALAR_OP(/, ETDDivision, e.dx(i) / scalar, -scalar* e.dx(i) / (e.f() * e.f()))

#define ETD_COMPARATOR(op)                                                                             \
    template <typename E1, typename E2>                                                                \
    DDATA_DEVICE_FUNCTION bool operator op(const ETDExpression<E1>& e1, const ETDExpression<E2>& e2) { \
        return e1.cast().f() op e2.cast().f();                                                         \
    }                                                                                                  \
    template <typename E>                                                                              \
    DDATA_DEVICE_FUNCTION bool operator op(typename E::value_type value, const ETDExpression<E>& e) {  \
        return value op e.cast().f();                                                                  \
    }                                                                                                  \
    template <typename E>                                                                              \
    DDATA_DEVICE_FUNCTION bool operator op(const ETDExpression<E>& e, typename E::value_type value) {  \
        return e.cast().f() op value;                                                                  \
    }

ETD_COMPARATOR(==)
ETD_COMPARATOR(!=)
ETD_COMPARATOR(<)
ETD_COMPARATOR(>)
ETD_COMPARATOR(<=)
ETD_COMPARATOR(>=)

template <typename E>
class ETDNegate : public ETDExpression<ETDNegate<E>> {
  public:
    DDATA_DEVICE_FUNCTION explicit ETDNegate(const E& e) : e(e) {}
    ETD_NO_COPY_CONSTRUCTOR(ETDNegate)
    using value_type = typename E::value_type;
    using derivative_type = typename E::derivative_type;
    const E& e;
    DDATA_DEVICE_FUNCTION value_type f() const { return -e.f(); }
    DDATA_DEVICE_FUNCTION derivative_type dx(int i) const { return -e.dx(i); }
    DDATA_DEVICE_FUNCTION int size() const { return e.size(); }
};
template <typename E>
std::ostream& operator<<(std::ostream& os, const ETDExpression<E>& e) {
    os << e.cast().f();
    return os;
}

template <typename E>
DDATA_DEVICE_FUNCTION auto operator-(const ETDExpression<E>& e) {
    return ETDNegate<E>(e.cast());
}
}
