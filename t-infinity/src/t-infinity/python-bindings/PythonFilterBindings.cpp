#include "PybindIncludes.h"

#include <t-infinity/FilterFactory.h>
#include <t-infinity/VizFromDictionary.h>
#include <t-infinity/StructuredMeshFilter.h>
#include <parfait/JsonParser.h>

using namespace inf;
namespace py = pybind11;

class PyFilter {
  public:
    PyFilter(std::shared_ptr<CellIdFilter> f) : filter(f) {}

    std::shared_ptr<MeshInterface> getMesh() { return filter->getMesh(); }
    auto apply(std::shared_ptr<FieldInterface> field) { return filter->apply(field); }

  private:
    std::shared_ptr<CellIdFilter> filter;
};

class PyIsoFilter {
  public:
    PyIsoFilter(std::shared_ptr<isosampling::Isosurface> f) : filter(f) {}

    std::shared_ptr<MeshInterface> getMesh() { return filter->getMesh(); }
    auto apply(std::shared_ptr<FieldInterface> field) { return filter->apply(field); }

  private:
    std::shared_ptr<isosampling::Isosurface> filter;
};

void PythonFilterBindings(py::module& m) {
    m.def("visualizeFromDictionary",
          [](MessagePasser mp,
             std::shared_ptr<MeshInterface> mesh,
             std::vector<std::shared_ptr<FieldInterface>> fields,
             std::string config) {
              visualizeFromDictionary(mp, mesh, fields, Parfait::JsonParser::parse(config));
          });

    py::class_<PyFilter>(m, "Filter")
        .def("getMesh", &PyFilter::getMesh)
        .def("apply", &PyFilter::apply);

    m.def("CutPlaneFilter",
          [](MessagePasser mp,
             std::shared_ptr<MeshInterface> mesh,
             std::array<double, 3> point_on_plane,
             std::array<double, 3> normal) {
              auto filter = std::make_shared<isosampling::Isosurface>(
                  mp.getCommunicator(), mesh, isosampling::plane(*mesh, point_on_plane, normal));
              return PyIsoFilter(filter);
          });

    m.def("ClipSphereFilter",
          [](MessagePasser mp,
             std::shared_ptr<MeshInterface> mesh,
             double r,
             std::vector<double> coords) {
              auto filter = FilterFactory::clipSphereFilter(
                  mp.getCommunicator(), mesh, r, {coords[0], coords[1], coords[2]});
              return PyFilter(filter);
          });

    m.def("ClipExtentFilter",
          [](MessagePasser mp,
             std::shared_ptr<MeshInterface> mesh,
             std::vector<double> lo,
             std::vector<double> hi) {
              Parfait::Extent<double> e({lo[0], lo[1], lo[2]}, {hi[0], hi[1], hi[2]});
              auto filter = FilterFactory::clipExtentFilter(mp.getCommunicator(), mesh, e);
              return PyFilter(filter);
          });

    m.def("SurfaceFilter", [](MessagePasser mp, std::shared_ptr<MeshInterface> mesh) {
        auto filter = FilterFactory::surfaceFilter(mp.getCommunicator(), mesh);
        return PyFilter(filter);
    });

    m.def("VolumeFilter", [](MessagePasser mp, std::shared_ptr<MeshInterface> mesh) {
        auto filter = FilterFactory::volumeFilter(mp.getCommunicator(), mesh);
        return PyFilter(filter);
    });

    m.def("SurfaceTagFilter",
          [](MessagePasser mp, std::shared_ptr<MeshInterface> mesh, const std::vector<int>& tags) {
              auto filter = FilterFactory::surfaceTagFilter(mp.getCommunicator(), mesh, tags);
              return PyFilter(filter);
          });

    m.def("CellTypeFilter",
          [](MessagePasser mp,
             std::shared_ptr<MeshInterface> mesh,
             const std::vector<MeshInterface::CellType>& types) {
              auto filter = FilterFactory::cellTypeFilter(mp.getCommunicator(), mesh, types);
              return PyFilter(filter);
          });

    m.def("TagFilter",
          [](MessagePasser mp, std::shared_ptr<MeshInterface> mesh, const std::vector<int>& tags) {
              auto filter = FilterFactory::tagFilter(mp.getCommunicator(), mesh, tags);
              return PyFilter(filter);
          });

    m.def("ValueFilter",
          [](MessagePasser mp,
             std::shared_ptr<MeshInterface> mesh,
             double lo,
             double hi,
             std::shared_ptr<FieldInterface> field) {
              auto filter = FilterFactory::valueFilter(mp.getCommunicator(), mesh, lo, hi, field);
              return PyFilter(filter);
          });

    typedef std::shared_ptr<StructuredMeshFilter> PyStructuredMeshFilter;
    py::class_<StructuredMeshFilter, PyStructuredMeshFilter>(m, "StructuredMeshFilter")
        .def("getMesh", &StructuredMeshFilter::getMesh)
        .def("apply", &StructuredMeshFilter::apply);

    py::bind_map<StructuredBlockSideSelector::BlockSidesToFilter>(m, "BlockSidesToFilter");
    m.def("BlockSideFilter",
          [](MessagePasser mp,
             std::shared_ptr<StructuredMesh> mesh,
             StructuredBlockSideSelector::BlockSidesToFilter block_sides_to_filter) {
              auto filter = StructuredBlockSideSelector(mp, block_sides_to_filter);
              return std::make_shared<StructuredMeshFilter>(mesh, filter);
          });
}
