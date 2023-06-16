#include "PybindIncludes.h"
#include <t-infinity/Version.h>

void PythonMPIBindings(pybind11::module& m);
void PythonMotionMatrixBindings(pybind11::module& m);
void PythonFilterBindings(pybind11::module& m);
void PythonVisualizationBindings(pybind11::module& m);
void PythonMeshBindings(pybind11::module& m);
void PythonFieldBindings(pybind11::module& m);
void PythonFluidSolverBindings(pybind11::module& m);
void PythonAdaptationBindings(pybind11::module& m);
void PythonDomainAssemblerBindings(pybind11::module& m);
void PythonSnapBindings(pybind11::module& m);
void PythonTracerBindings(pybind11::module& m);

PYBIND11_MODULE(_infinity_plugins, m) {
    PythonMPIBindings(m);
    PythonMotionMatrixBindings(m);
    PythonFilterBindings(m);
    PythonVisualizationBindings(m);
    PythonMeshBindings(m);
    PythonFieldBindings(m);
    PythonFluidSolverBindings(m);
    PythonAdaptationBindings(m);
    PythonDomainAssemblerBindings(m);
    PythonSnapBindings(m);
    PythonTracerBindings(m);
    m.def("version", &inf::version);
}
