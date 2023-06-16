#include "PybindIncludes.h"

#include <parfait/MotionMatrix.h>

void PythonMotionMatrixBindings(pybind11::module& m) {
    pybind11::class_<Parfait::MotionMatrix>(m, "Motion")
        .def(pybind11::init())
        .def("addTranslation",
             [](Parfait::MotionMatrix& self, std::array<double, 3> translation) {
                 self.addTranslation(translation.data());
             })
        .def("addRotation",
             [](Parfait::MotionMatrix& self,
                std::array<double, 3> axis_begin,
                std::array<double, 3> axis_end,
                double degrees) { self.addRotation(axis_begin.data(), axis_end.data(), degrees); })
        .def("move", [](Parfait::MotionMatrix& self, double* point) { self.movePoint(point); })
        .def("toString", [](Parfait::MotionMatrix& self) { return self.toString(); });
}
