#pragma once
#include <array>
#include <vector>
#include "Facet.h"

namespace Parfait {
inline std::vector<Parfait::Facet> facetizeTet(const std::array<Parfait::Point<double>, 4>& tet) {
    std::vector<Parfait::Facet> facets;
    facets.emplace_back(tet[0], tet[1], tet[2]);
    facets.emplace_back(tet[0], tet[3], tet[1]);
    facets.emplace_back(tet[0], tet[2], tet[3]);
    facets.emplace_back(tet[1], tet[3], tet[2]);
    return facets;
}

inline std::vector<Parfait::Facet> facetizeHex(const std::array<Parfait::Point<double>, 8>& hex) {
    std::vector<Parfait::Facet> facets;
    facets.emplace_back(hex[0], hex[1], hex[2]);
    facets.emplace_back(hex[2], hex[3], hex[0]);

    facets.emplace_back(hex[0], hex[4], hex[5]);
    facets.emplace_back(hex[5], hex[1], hex[0]);

    facets.emplace_back(hex[1], hex[5], hex[6]);
    facets.emplace_back(hex[6], hex[2], hex[1]);

    facets.emplace_back(hex[2], hex[6], hex[7]);
    facets.emplace_back(hex[7], hex[3], hex[2]);

    facets.emplace_back(hex[0], hex[3], hex[7]);
    facets.emplace_back(hex[7], hex[4], hex[0]);

    facets.emplace_back(hex[4], hex[7], hex[6]);
    facets.emplace_back(hex[6], hex[5], hex[4]);
    return facets;
}
}
