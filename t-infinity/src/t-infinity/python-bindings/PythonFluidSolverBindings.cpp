#include "PybindIncludes.h"

#include <t-infinity/FluidSolverTools.h>
#include <parfait/VectorTools.h>

using namespace inf;
namespace py = pybind11;

void PythonFluidSolverBindings(py::module& m) {
    m.def("writeSolverFieldsToSnap", &writeSolverFieldsToSnap);
    m.def("writeSolutionToSnap", &writeSolutionToSnap);
    m.def("setSolutionFromSnap", &setSolutionFromSnap);
    m.def("setSolutionFromFields", &setFluidSolverSolution);
    m.def("extractScalarsAsNodeField",
          [](std::shared_ptr<FluidSolverInterface> solver,
             std::shared_ptr<MeshInterface> mesh,
             const std::string& field_name,
             const std::vector<std::string>& scalar_names) {
              return extractScalarsAsNodeField(*solver, *mesh, field_name, scalar_names);
          });
    m.def("extractScalarsAsCellField",
          [](std::shared_ptr<FluidSolverInterface> solver,
             std::shared_ptr<MeshInterface> mesh,
             const std::string& field_name,
             const std::vector<std::string>& scalar_names) {
              return extractScalarsAsCellField(*solver, *mesh, field_name, scalar_names);
          });

    typedef std::shared_ptr<FluidSolverInterface> FluidSolver;
    py::class_<FluidSolverInterface, FluidSolver>(m, "FluidSolver")
        .def(py::init([](std::shared_ptr<MeshInterface> mesh,
                         MessagePasser mp,
                         std::string json_config,
                         std::string directory,
                         std::string name) {
            auto solver_factory = getFluidSolverFactory(directory, name);
            return solver_factory->createFluidSolver(
                mesh, std::move(json_config), mp.getCommunicator());
        }))
        .def("solve", &FluidSolverInterface::solve)
        .def("step", &FluidSolverInterface::step)
        .def("freezeNodes", &FluidSolverInterface::freezeNodes)
        .def("freezeCells", &FluidSolverInterface::freezeCells)
        .def("setSolutionAtNode",
             [](FluidSolver self, int node_id, const std::vector<double>& v) {
                 self->setSolutionAtNode(node_id, v.data());
             })
        .def("setSolutionAtCell",
             [](FluidSolver self, int cell_id, const std::vector<double>& v) {
                 self->setSolutionAtCell(cell_id, v.data());
             })
        .def("getScalarAtNode", &FluidSolverInterface::getScalarAtNode)
        .def("getScalarAtCell", &FluidSolverInterface::getScalarAtCell)
        .def("expectsSolutionAs", &FluidSolverInterface::expectsSolutionAs)
        .def("listScalarsAtNodes", &FluidSolverInterface::listScalarsAtNodes)
        .def("listScalarsAtCells", &FluidSolverInterface::listScalarsAtCells)
        .def("listFields",
             [](FluidSolver self) {
                 auto fields = self->listScalarsAtNodes();
                 Parfait::VectorTools::append(fields, self->listScalarsAtCells());
                 return fields;
             })
        .def("unloadPlugin", [](FluidSolver self) { self.reset(); });
}
