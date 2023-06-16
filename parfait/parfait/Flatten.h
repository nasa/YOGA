#pragma once

namespace Parfait {
template <typename T>
std::vector<T> flatten(const std::vector<std::vector<T>>& things) {
    std::vector<T> out;
    for (const auto& inner_vector : things) {
        for (const auto& t : inner_vector) {
            out.push_back(t);
        }
    }
    return out;
}

}
