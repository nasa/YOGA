#include "PybindIncludes.h"

#include <t-infinity/Snap.h>

using namespace inf;
namespace py = pybind11;

void PythonSnapBindings(py::module& m) {
    typedef std::shared_ptr<Snap> PySnap;
    py::class_<Snap, PySnap>(m, "Snap")
        .def(py::init([](std::shared_ptr<MeshInterface> mesh, MessagePasser mp) {
            auto snap = std::make_shared<Snap>(mp.getCommunicator());
            snap->addMeshTopologies(*mesh);
            return snap;
        }))
        .def(py::init(
            [](std::string filename, std::shared_ptr<MeshInterface> mesh, MessagePasser mp) {
                auto snap = std::make_shared<Snap>(mp.getCommunicator());
                snap->addMeshTopologies(*mesh);
                snap->load(filename);
                return snap;
            }))
        .def("add",
             [](PySnap self, std::shared_ptr<FieldInterface> field) {
                 if (1 != field->blockSize())
                     throw std::logic_error("Python Snap only supports scalar fields");
                 self->add(field);
             })
        .def("listFields", &Snap::availableFields)
        .def("field", &Snap::retrieve)
        .def("write", &Snap::writeFile);
}
