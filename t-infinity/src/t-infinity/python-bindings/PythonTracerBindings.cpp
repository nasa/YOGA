#include "PybindIncludes.h"

#include <utility>
#include <Tracer.h>

namespace py = pybind11;

void PythonTracerBindings(py::module& m) {
    m.def("initializeTracer",
          [](std::string filename_base, int process_id, std::vector<int> ranks_to_trace) {
              Tracer::initialize(std::move(filename_base), process_id, std::move(ranks_to_trace));
          });
    m.def("finalizeTracer", &Tracer::finalize);
    m.def("TracerBegin", &Tracer::begin);
    m.def("TracerEnd", &Tracer::end);
    m.def("TracerSetDebug", &Tracer::setDebug);
    m.def("TracerSetFast", &Tracer::setFast);
    m.def("TracerIsInitialized", &Tracer::isInitialized);
    m.def("TracerIsEnabled", &Tracer::isEnabled);
}
