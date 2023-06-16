#pragma once

#include <catch2/catch_all.hpp>
using Catch::Approx;
using Catch::Matchers::EndsWith;
using Catch::Matchers::StartsWith;

template <typename T>
auto Contains(T thing) {
    return Catch::Matchers::Contains(thing);
}

template <>
inline auto Contains<std::string>(std::string thing) {
    return Catch::Matchers::ContainsSubstring(thing);
}

template <>
inline auto Contains<const char*>(const char* thing) {
    return Catch::Matchers::ContainsSubstring(thing);
}
