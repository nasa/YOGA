#pragma once

#include "Stack.h"
#include "Expr.h"
#include "StackOperators.h"
#include "StackFunctions.h"

namespace Linearize {
struct adtype : Expr<adtype> {
    adtype() : value(0.0), gradient_index(ACTIVE_STACK.newGradient()) {}
    adtype(double v) : value(v), gradient_index(ACTIVE_STACK.newGradient()) {
        ACTIVE_STACK.addStatement(gradient_index);
    }
    adtype(const adtype& rhs) : value(rhs.value), gradient_index(ACTIVE_STACK.newGradient()) {
        rhs.calcGradient(ACTIVE_STACK, 1.0);
        ACTIVE_STACK.addStatement(gradient_index);
    }
    template <class Var>
    adtype(const Expr<Var>& rhs) : value(rhs.cast().f()), gradient_index(ACTIVE_STACK.newGradient()) {
        rhs.cast().calcGradient(ACTIVE_STACK, 1.0);
        ACTIVE_STACK.addStatement(gradient_index);
    }
    adtype& operator=(const adtype& rhs) {
        rhs.cast().calcGradient(ACTIVE_STACK, 1.0);
        ACTIVE_STACK.addStatement(gradient_index);
        value = rhs.value;
        return *this;
    }
    template <class Var>
    adtype& operator=(const Expr<Var>& rhs) {
        rhs.cast().calcGradient(ACTIVE_STACK, 1.0);
        ACTIVE_STACK.addStatement(gradient_index);
        value = rhs.cast().f();
        return *this;
    }
    adtype& operator=(double rhs) {
        value = rhs;
        ACTIVE_STACK.addStatement(gradient_index);
        return *this;
    }
    template <class Var>
    adtype& operator+=(const Expr<Var>& rhs) {
        return *this = (*this + rhs);
    }
    template <class Var>
    adtype& operator-=(const Expr<Var>& rhs) {
        return *this = (*this - rhs);
    }
    template <class Var>
    adtype& operator*=(const Expr<Var>& rhs) {
        return *this = (*this * rhs);
    }
    template <class Var>
    adtype& operator/=(const Expr<Var>& rhs) {
        return *this = (*this / rhs);
    }
    adtype& operator+=(const double& rhs) {
        value += rhs;
        return *this;
    }
    adtype& operator-=(const double& rhs) {
        value -= rhs;
        return *this;
    }
    adtype& operator*=(const double& rhs) {
        value *= rhs;
        return *this;
    }
    adtype& operator/=(const double& rhs) {
        value /= rhs;
        return *this;
    }
    double f() const { return value; }

    void setGradient(double gradient) const {
        ACTIVE_STACK.gradients.resize(ACTIVE_STACK.num_gradients, 0.0);
        ACTIVE_STACK.gradients[gradient_index] = gradient;
    }
    const double& getGradient() const { return ACTIVE_STACK.gradients.at(gradient_index); }
    void calcGradient(Stack& s, const double& multiplier) const { s.addOperation(multiplier, gradient_index); }
    virtual ~adtype() { ACTIVE_STACK.releaseGradient(gradient_index); }
    double value;
    int gradient_index;
};

using Var = adtype;
}
