#pragma once

#include "CellIdFilter.h"
#include "MeshConnectivity.h"

namespace inf {

class FilterFactory {
  public:
    typedef MPI_Comm Comm;
    typedef std::shared_ptr<MeshInterface> Mesh;
    typedef std::shared_ptr<FieldInterface> Field;
    typedef const Parfait::Point<double>& Point;
    typedef const Parfait::Extent<double>& Extent;
    typedef std::shared_ptr<CellIdFilter> Filter;

    inline static Filter clipSphereFilter(Comm comm, Mesh mesh, double radius, Point p) {
        auto selector = std::make_shared<SphereSelector>(radius, p);
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    inline static Filter clipPlaneFilter(Comm comm, Mesh mesh, Point center, Point normal) {
        auto selector = std::make_shared<PlaneSelector>(center, normal);
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    inline static Filter clipLineSegmentFilter(Comm comm, Mesh mesh, Point a, Point b) {
        auto selector = std::make_shared<LineSegmentSelector>(a, b);
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    inline static Filter clipExtentFilter(Comm comm, Mesh mesh, Extent e) {
        auto selector = std::make_shared<ExtentSelector>(e);
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    inline static Filter surfaceFilter(Comm comm, Mesh mesh) {
        auto selector = std::make_shared<DimensionalitySelector>(2);
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    inline static Filter volumeFilter(Comm comm, Mesh mesh) {
        auto selector = std::make_shared<DimensionalitySelector>(3);
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    inline static Filter cellTypeFilter(Comm comm,
                                        Mesh mesh,
                                        const std::vector<MeshInterface::CellType>& types) {
        auto selector = std::make_shared<CellTypeSelector>(types);
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    inline static Filter tagFilter(Comm comm, Mesh mesh, const std::vector<int>& tags) {
        auto selector = std::make_shared<TagSelector>(tags);
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    inline static Filter tagFilter(Comm comm, Mesh mesh, const std::set<int>& tags) {
        auto selector = std::make_shared<TagSelector>(tags);
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    inline static Filter nodeValueFilter(Comm comm, Mesh mesh, double lo, double hi, Field field) {
        std::vector<double> values(mesh->nodeCount());
        for (int i = 0; i < mesh->nodeCount(); i++) field->value(i, &values[i]);
        auto selector = std::make_shared<NodeValueSelector>(lo, hi, std::move(values));
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    inline static Filter cellValueFilter(Comm comm, Mesh mesh, double lo, double hi, Field field) {
        std::vector<double> values(mesh->cellCount());
        for (int i = 0; i < mesh->cellCount(); i++) field->value(i, &values[i]);
        auto selector = std::make_shared<CellValueSelector>(lo, hi, std::move(values));
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    inline static Filter valueFilter(Comm comm, Mesh mesh, double lo, double hi, Field field) {
        if (FieldAttributes::Node() == field->association())
            return nodeValueFilter(comm, mesh, lo, hi, field);
        else if (FieldAttributes::Cell() == field->association())
            return cellValueFilter(comm, mesh, lo, hi, field);
        else
            throw std::logic_error("Value filtering not supported for type " +
                                   field->association());
    }

    inline static Filter surfaceTagFilter(Comm comm, Mesh mesh, const std::vector<int>& tags) {
        auto selector = std::make_shared<CompositeSelector>();
        selector->add(std::make_shared<DimensionalitySelector>(2));
        selector->add(std::make_shared<TagSelector>(tags));
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }
    inline static Filter surfaceTagFilter(Comm comm, Mesh mesh, const std::set<int>& tags) {
        auto selector = std::make_shared<CompositeSelector>();
        selector->add(std::make_shared<DimensionalitySelector>(2));
        selector->add(std::make_shared<TagSelector>(tags));
        return std::make_shared<CellIdFilter>(comm, mesh, selector);
    }

    template <typename Container>
    static Filter nodeStencilFilter(Comm comm,
                                    Mesh mesh,
                                    const Container& stencil_center_nodes,
                                    const std::vector<std::vector<int>>& node_to_cell) {
        std::set<int> cells_to_include;
        for (int node : stencil_center_nodes) {
            for (auto neighbor_cell : node_to_cell[node]) cells_to_include.insert(neighbor_cell);
        }
        return std::make_shared<CellIdFilter>(
            comm, mesh, std::vector<int>(cells_to_include.begin(), cells_to_include.end()));
    }

    template <typename Container>
    static Mesh filterToNodalStencils(Comm comm,
                                      Mesh mesh,
                                      const Container& stencil_center_nodes,
                                      const std::vector<std::vector<int>>& node_to_cell) {
        auto filter = nodeStencilFilter(comm, mesh, stencil_center_nodes, node_to_cell);
        return filter->getMesh();
    }

    template <typename Container>
    static Mesh filterToNodalStencils(Comm comm, Mesh mesh, const Container& stencil_center_nodes) {
        return filterToNodalStencils(comm, mesh, stencil_center_nodes, NodeToCell::build(*mesh));
    }
};

}
