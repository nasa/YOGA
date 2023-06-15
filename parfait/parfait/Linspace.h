#pragma once
#include <parfait/Point.h>

namespace Parfait {
template <typename T>
std::vector<T> linspace(T start, T end, size_t num_items) {
    std::vector<T> out(num_items);
    auto dx = (end - start) / static_cast<double>(num_items - 1);
    for (size_t i = 0; i < num_items; i++) {
        out[i] = start + dx * i;
    }
    return out;
}
}
