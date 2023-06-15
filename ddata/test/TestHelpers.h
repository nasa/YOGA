#pragma once
#include <ddata/ETD.h>
#include <ddata/DTD.h>

namespace Linearize {
template <class ADType, typename Functor>
std::vector<typename ADType::value_type> calcDerivatives(const ADType& ddt, Functor func) {
    using T = typename ADType::value_type;
    std::vector<T> dx(ddt.size());
    for (int i = 0; i < ddt.size(); ++i) {
        dx[i] = func(ddt.dx(i));
    }
    return dx;
}

template <class ADType, typename Functor>
std::vector<typename ADType::value_type> calcDerivatives(const ADType& ddt1, const ADType& ddt2, Functor func) {
    assert(ddt1.size() == ddt2.size());
    using T = typename ADType::value_type;
    std::vector<T> dx(ddt1.size());
    for (int i = 0; i < ddt1.size(); ++i) {
        dx[i] = func(ddt1.dx(i), ddt2.dx(i));
    }
    return dx;
}

template <size_t N, typename T, template <size_t, typename> class ADType>
auto extractDerivatives(const ADType<N, T>& ddt) {
    std::vector<T> dx(N);
    for (size_t i = 0; i < N; ++i) {
        dx[i] = ddt.dx(i);
    }
    return dx;
}

template <class ADType>
auto extractDerivatives(const ADType& ddt) {
    std::vector<typename ADType::value_type> dx(ddt.size());
    for (int i = 0; i < ddt.size(); ++i) {
        dx[i] = ddt.dx(i);
    }
    return dx;
}
}
