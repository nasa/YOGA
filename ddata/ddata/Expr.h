#pragma once

namespace Linearize {
template <typename Var>
struct Expr {
    const Var& cast() const { return static_cast<const Var&>(*this); }
};
}
