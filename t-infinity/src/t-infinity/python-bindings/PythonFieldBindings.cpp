#include "PybindIncludes.h"
#include <pybind11/functional.h>

#include <t-infinity/FieldInterface.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/FieldShuffle.h>
#include <t-infinity/SMesh.h>
#include <parfait/Flatten.h>

using namespace inf;
namespace py = pybind11;

void PythonFieldBindings(py::module& m) {
    m.def("NodeAssociation", []() { return inf::FieldAttributes::Node(); });
    m.def("CellAssociation", []() { return inf::FieldAttributes::Cell(); });

    typedef std::shared_ptr<FieldInterface> Field;
    py::class_<FieldInterface, Field>(m, "Field")
        .def(py::init([](std::string name, const std::vector<double>& scalar) {
            return std::make_shared<VectorFieldAdapter>(
                name, FieldAttributes::Unassigned(), 1, scalar);
        }))
        .def(py::init(
            [](std::string name, std::string association, const std::vector<double>& scalar) {
                return std::make_shared<VectorFieldAdapter>(name, association, 1, scalar);
            }))
        .def("association", [](Field self) { return self->association(); })
        .def("name", &FieldInterface::name)
        .def("at", &FieldInterface::at)
        .def("size", &FieldInterface::size)
        .def("blockSize", &FieldInterface::blockSize)
        .def("max", &FieldInterface::max)
        .def("norm", &FieldInterface::norm)
        .def(
            "__add__",
            [](Field self, Field b) { return FieldTools::sum(*self, *b); },
            py::is_operator())
        .def(
            "__sub__",
            [](Field self, Field b) { return FieldTools::diff(*self, *b); },
            py::is_operator())
        .def(
            "__mul__",
            [](Field self, Field b) { return FieldTools::multiply(*self, *b); },
            py::is_operator())
        .def(
            "__truediv__",
            [](Field self, Field b) { return FieldTools::divide(*self, *b); },
            py::is_operator())
        .def(
            "__add__",
            [](Field self, double b) { return FieldTools::sum(*self, b); },
            py::is_operator())
        .def(
            "__sub__",
            [](Field self, double b) { return FieldTools::diff(*self, b); },
            py::is_operator())
        .def(
            "__mul__",
            [](Field self, double b) { return FieldTools::multiply(*self, b); },
            py::is_operator())
        .def(
            "__truediv__",
            [](Field self, double b) { return FieldTools::divide(*self, b); },
            py::is_operator())
        .def("globalMin",
             [](Field self, MessagePasser mp, std::shared_ptr<MeshInterface> mesh) {
                 return FieldTools::findMin(mp, *mesh, *self);
             })
        .def("globalMax",
             [](Field self, MessagePasser mp, std::shared_ptr<MeshInterface> mesh) {
                 return FieldTools::findMax(mp, *mesh, *self);
             })
        .def("averageToNodes",
             [](Field self, std::shared_ptr<MeshInterface> mesh) {
                 return FieldTools::cellToNode_volume(*mesh, *self);
             })
        .def("averageToSurfaceNodes",
             [](Field self, std::shared_ptr<MeshInterface> mesh) {
                 return FieldTools::cellToNode_surface(*mesh, *self);
             })
        .def("averageToCells",
             [](Field self, std::shared_ptr<MeshInterface> mesh) {
                 return FieldTools::nodeToCell(*mesh, *self);
             })
        .def("taubinSmoothing",
             [](Field self, std::shared_ptr<MeshInterface> mesh, MessagePasser mp, int passes) {
                 Parfait::Dictionary opts;
                 opts["passes"] = passes;
                 return FieldTools::smoothField(mp, "taubin", self, mesh, opts);
             })
        .def("laplacianSmoothing",
             [](Field self, std::shared_ptr<MeshInterface> mesh, MessagePasser mp, int passes) {
                 Parfait::Dictionary opts;
                 opts["passes"] = passes;
                 return FieldTools::smoothField(mp, "laplacian", self, mesh, opts);
             });

    typedef std::shared_ptr<StructuredBlockField> PyStructuredBlockField;
    py::class_<StructuredBlockField, PyStructuredBlockField>(m, "StructuredBlockField")
        .def(py::init([](std::array<int, 3> dimensions, int block_id) {
            return SField::createBlock(dimensions, block_id);
        }))
        .def("dimensions", &StructuredBlockField::dimensions)
        .def("setValue", &StructuredBlockField::setValue)
        .def("value", &StructuredBlockField::value)
        .def("blockId", &StructuredBlockField::blockId);

    typedef std::shared_ptr<StructuredField> PyStructuredField;
    py::class_<StructuredField, PyStructuredField>(m, "StructuredField")
        .def(py::init([](std::string name, std::string association) {
            return std::make_shared<SField>(name, association);
        }))
        .def("name", &StructuredField::name)
        .def("blockIds", &StructuredField::blockIds)
        .def("getBlockField", &StructuredField::getBlockField)
        .def("add",
             [](PyStructuredField self, PyStructuredBlockField block) {
                 std::dynamic_pointer_cast<SField>(self)->add(block);
             })
        .def("createBlock", &SField::createBlock)
        .def("association", &StructuredField::association);

    m.def("createVectorField",
          [](std::string name,
             std::string association,
             int block_size,
             std::vector<std::vector<double>> scalars) -> Field {
              return std::make_shared<VectorFieldAdapter>(
                  name, association, block_size, Parfait::flatten(scalars));
          });

    m.def("calcSpaldingUPlus",
          [](double target_spalding_yplus, std::vector<double> wall_distance) -> Field {
              auto uplus =
                  FieldTools::calcSpaldingLawOfTheWallUPlus(target_spalding_yplus, wall_distance);
              return std::make_shared<VectorFieldAdapter>(
                  "spalding uplus", FieldAttributes::Node(), uplus);
          });
    m.def("binaryFieldOp",
          [](std::string name, Field f1, Field f2, FieldTools::FieldOperation operation) {
              return FieldTools::op(name, *f1, *f2, operation);
          });
    m.def("scalarFieldOp",
          [](std::string name, Field f, FieldTools::ScalarFieldOperation operation) {
              return FieldTools::op(name, *f, operation);
          });
    m.def("shuffleField",
          [](MessagePasser mp,
             std::shared_ptr<MeshInterface> source,
             std::shared_ptr<MeshInterface> target,
             Field source_field) {
              return FieldShuffle::shuffleNodeField<double>(mp, *source, *target, *source_field);
          });
}
