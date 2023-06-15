#pragma once

#include <parfait/ExtentBuilder.h>
#include <parfait/Facet.h>
#include <parfait/Point.h>
#include <parfait/UniquePoints.h>

namespace Parfait {
template <typename GetFacet>
Parfait::Extent<double> buildDomain(GetFacet& getFacet, int num_facets) {
    auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    for (int i = 0; i < num_facets; i++) {
        auto f = getFacet(i);
        Parfait::ExtentBuilder::add(e, f[0]);
        Parfait::ExtentBuilder::add(e, f[1]);
        Parfait::ExtentBuilder::add(e, f[2]);
    }
    return e;
}

template <typename GetCell>
std::vector<int> stitchCells(Parfait::UniquePoints& unique_points, GetCell& getCell, int num_cells, int cell_length) {
    std::vector<int> cells;
    std::vector<int> cell_nodes;
    for (int c_id = 0; c_id < num_cells; c_id++) {
        auto cell = getCell(c_id);
        cell_nodes.resize(cell_length);
        for (int i = 0; i < cell_length; i++) {
            cell_nodes[i] = unique_points.getNodeId(cell[i]);
        }
        if (std::set<int>(cell_nodes.begin(), cell_nodes.end()).size() == size_t(cell_length)) {
            for (int i = 0; i < cell_length; i++) {
                cells.push_back(cell_nodes[i]);
            }
        }
    }
    return cells;
}

template <typename GetFacet>
std::tuple<std::vector<int>, std::vector<Parfait::Point<double>>> stitchFacets(GetFacet& getFacet, int num_facets) {
    auto domain = buildDomain(getFacet, num_facets);
    Parfait::UniquePoints unique_points(domain);
    auto triangles = stitchCells(unique_points, getFacet, num_facets, 3);
    return {triangles, unique_points.points};
}

inline std::tuple<std::vector<int>, std::vector<Parfait::Point<double>>> stitchFacets(
    const std::vector<Parfait::Facet>& facets) {
    auto getFacet = [&](int i) { return facets[i]; };

    return stitchFacets(getFacet, facets.size());
}

inline std::tuple<std::vector<int>, std::vector<Parfait::Point<double>>> stitchExtents(
    const std::vector<Parfait::Extent<double>>& extents) {
    auto domain = Parfait::ExtentBuilder::getBoundingBox(extents);
    Parfait::UniquePoints unique_points(domain);

    auto getHex = [&](int i) { return extents.at(i).corners(); };
    auto hexs = stitchCells(unique_points, getHex, extents.size(), 8);
    return {hexs, unique_points.points};
}
}
