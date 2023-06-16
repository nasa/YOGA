#pragma once

#include <vector>

namespace Parfait {
template <typename T>
inline void laplacianSmoothing(T zero, const std::vector<std::vector<int>>& n2n, std::vector<T>& field) {
    PARFAIT_ASSERT(field.size() == n2n.size(), "field and connectivity mismatch");
    auto f = field;
    for (size_t i = 0; i < field.size(); ++i) {
        field[i] = zero;
        for (int n : n2n[i]) field[i] += f[n];
        field[i] /= double(n2n[i].size());
    }
}
}