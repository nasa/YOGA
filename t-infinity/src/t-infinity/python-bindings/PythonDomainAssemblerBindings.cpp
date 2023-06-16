#include "PybindIncludes.h"

#include <t-infinity/DomainAssemblerInterface.h>
#include <t-infinity/Communicator.h>
#include <t-infinity/PluginLoader.h>
#include <t-infinity/FluidSolverInterface.h>

using namespace inf;
namespace py = pybind11;

void PythonDomainAssemblerBindings(py::module& m) {
    typedef std::shared_ptr<DomainAssemblerInterface> Assembler;
    py::class_<DomainAssemblerInterface, Assembler>(m, "DomainAssembler")
        .def(py::init([](std::shared_ptr<MeshInterface> mesh,
                         MessagePasser mp,
                         int component_id,
                         std::string directory,
                         std::string name,
                         std::string json_config) {
            auto factory = getDomainAssemblerFactory(directory, name);
            return factory->createDomainAssembler(
                mp.getCommunicator(), *mesh, component_id, std::move(json_config));
        }))
        .def("unloadPlugin", [](Assembler self) { self.reset(); })
        .def("performDomainAssembly", &DomainAssemblerInterface::performAssembly)
        .def("listFields", &DomainAssemblerInterface::listFields)
        .def("determineGridIdsForNodes", &DomainAssemblerInterface::determineGridIdsForNodes)
        .def("field", &DomainAssemblerInterface::field)
        .def("updateReceptorSolutions",
             [](Assembler self, std::shared_ptr<FluidSolverInterface> solver) {
                 auto call_back = [&](int cell_id, double x, double y, double z, double* solution) {
                     solver->solutionInCell(x, y, z, cell_id, solution);
                 };
                 return self->updateReceptorSolutions(call_back);
             });
}
