#pragma once

#include <set>
#include <t-infinity/MeshInterface.h>
#include <parfait/Extent.h>
#include "FilterTools.h"
#include "ExtentIntersections.h"
#include "IsoSampling.h"

namespace inf {

class CellSelector {
  public:
    virtual bool keep(const MeshInterface& mesh, int cell_id) const = 0;
    virtual ~CellSelector() = default;
};

class DimensionalitySelector : public CellSelector {
  public:
    DimensionalitySelector(int dimensionality) : dimensionality(dimensionality) {}
    bool keep(const MeshInterface& mesh, int cell_id) const {
        return mesh.cellDimensionality(cell_id) == dimensionality;
    }
    int dimensionality;
};

class CellTypeSelector : public CellSelector {
  public:
    CellTypeSelector(MeshInterface::CellType type) { types.insert(type); }
    CellTypeSelector(const std::vector<MeshInterface::CellType>& types)
        : types(types.begin(), types.end()) {}
    bool keep(const MeshInterface& mesh, int cell_id) const {
        auto cell_type = mesh.cellType(cell_id);
        return 1 == types.count(cell_type);
    }

  private:
    std::set<MeshInterface::CellType> types;
};

class GlobalCellSelector : public CellSelector {
  public:
    GlobalCellSelector(const std::vector<long>& global_cell_ids)
        : included_gcids(global_cell_ids.begin(), global_cell_ids.end()) {}
    GlobalCellSelector(const std::set<long>& global_cell_ids) : included_gcids(global_cell_ids) {}
    bool keep(const MeshInterface& mesh, int cell_id) const {
        auto gid = mesh.globalCellId(cell_id);
        return included_gcids.count(gid) == 1;
    }

  private:
    std::set<long> included_gcids;
};

class ExtentSelector : public CellSelector {
  public:
    ExtentSelector(const Parfait::Extent<double>& e) : selection_extent(e) {}
    bool keep(const MeshInterface& mesh, int cell_id) const {
        auto vertices = filter_tools::getPointsInCell(mesh, cell_id);
        auto cell_extent = Parfait::ExtentBuilder::build(vertices);
        return selection_extent.intersects(cell_extent);
    }

  private:
    const Parfait::Extent<double> selection_extent;
};

class SphereSelector : public CellSelector {
  public:
    SphereSelector(double radius, const Parfait::Point<double>& p) : center(p), radius(radius) {}
    bool keep(const MeshInterface& mesh, int cell_id) const {
        auto vertices = filter_tools::getPointsInCell(mesh, cell_id);
        auto cell_extent = Parfait::ExtentBuilder::build(vertices);
        return sphereExtentIntersection(radius, center, cell_extent);
    }

  private:
    const Parfait::Point<double> center;
    const double radius;
};

class PlaneSelector : public CellSelector {
  public:
    PlaneSelector(const Parfait::Point<double>& c, const Parfait::Point<double>& n)
        : center(c), normal(n) {
        normal.normalize();
    }
    bool keep(const MeshInterface& mesh, int cell_id) const {
        std::vector<int> nodes;
        mesh.cell(cell_id, nodes);
        bool behind = false;
        bool ahead = false;
        auto plane_sampling = isosampling::plane(mesh, center, normal);
        for (auto n : nodes) {
            if (plane_sampling(n) < 0.0) {
                behind = true;
            }
            if (plane_sampling(n) > 0.0) {
                ahead = true;
            }
        }
        return behind and ahead;
    }

  private:
    Parfait::Point<double> center;
    Parfait::Point<double> normal;
};

class ValueSelector : public CellSelector {
  protected:
    ValueSelector(double lo, double hi, const std::vector<double>& values)
        : lo(lo), hi(hi), values(values) {}
    const double lo, hi;
    std::vector<double> values;
};

class NodeValueSelector : public ValueSelector {
  public:
    NodeValueSelector(double lo, double hi, const std::vector<double>& v)
        : ValueSelector(lo, hi, v) {}
    bool keep(const MeshInterface& mesh, int cell_id) const override {
        std::vector<int> cell(mesh.cellLength(cell_id));
        mesh.cell(cell_id, cell.data());
        for (int id : cell)
            if (values[id] >= lo and values[id] <= hi) return true;
        return false;
    }
};

class CellValueSelector : public ValueSelector {
  public:
    CellValueSelector(double lo, double hi, const std::vector<double>& v)
        : ValueSelector(lo, hi, v) {}
    bool keep(const MeshInterface& mesh, int cell_id) const {
        return values[cell_id] >= lo and values[cell_id] <= hi;
    }
};

class TagSelector : public CellSelector {
  public:
    TagSelector(const std::vector<int>& tags) : tags(tags.begin(), tags.end()) {}
    TagSelector(const std::set<int>& t) : tags(t) {}
    bool keep(const MeshInterface& mesh, int cell_id) const {
        return tags.count(mesh.cellTag(cell_id)) == 1;
    }

  private:
    std::set<int> tags;
};

class LineSegmentSelector : public CellSelector {
  public:
    LineSegmentSelector(Parfait::Point<double> a_in, Parfait::Point<double> b_in)
        : a(a_in), b(b_in) {}
    bool keep(const MeshInterface& mesh, int cell_id) const {
        auto vertices = filter_tools::getPointsInCell(mesh, cell_id);
        auto cell_extent = Parfait::ExtentBuilder::build(vertices);
        return cell_extent.intersectsSegment(a, b);
    }

  private:
    Parfait::Point<double> a;
    Parfait::Point<double> b;
};

class CompositeSelector : public CellSelector {
  public:
    virtual bool keep(const MeshInterface& mesh, int cell_id) const {
        for (auto selector : selectors)
            if (not selector->keep(mesh, cell_id)) return false;
        return true;
    }

    void add(std::shared_ptr<CellSelector> selector) { selectors.push_back(selector); }

    int numSelectors() const { return int(selectors.size()); }

  protected:
    std::vector<std::shared_ptr<CellSelector>> selectors;
};

}
