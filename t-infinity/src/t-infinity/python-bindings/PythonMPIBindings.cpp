#include "PybindIncludes.h"

#include <t-infinity/Communicator.h>
#include <t-infinity/LoadBalancer.h>
#include <chrono>
#include <thread>

using namespace inf;
namespace py = pybind11;

void PythonMPIBindings(py::module& m) {
    m.def("initializeMPI", &initializeMPI);
    m.def("finalizeMPI", &finalizeMPI);
    m.def("getCommunicator", []() { return MessagePasser(MPI_COMM_WORLD); });
    m.def("splitCommunicator", [](MessagePasser mp, int color) {
        return MessagePasser(mp.split(mp.getCommunicator(), color));
    });
    m.def("assignColorByWeight", [](MessagePasser mp, std::vector<double> weights) {
        DivineLoadBalancer balancer(mp.Rank(), mp.NumberOfProcesses());
        return balancer.getAssignedDomain(weights);
    });

    py::class_<MessagePasser>(m, "MessagePasser")
        .def("sum", [](MessagePasser& self, long x) { return self.ParallelSum(x); })
        .def("sum", [](MessagePasser& self, double x) { return self.ParallelSum(x); })
        .def("max", [](MessagePasser& self, long x) { return self.ParallelMax(x); })
        .def("max", [](MessagePasser& self, double x) { return self.ParallelMax(x); })
        .def("min", [](MessagePasser& self, long x) { return self.ParallelMin(x); })
        .def("min", [](MessagePasser& self, double x) { return self.ParallelMin(x); })
        .def("barrier", [](MessagePasser& self) { self.Barrier(); })
        .def("abort", [](MessagePasser& self) { self.Abort(); })
        .def("rank", &MessagePasser::Rank)
        .def("numberOfRanks", &MessagePasser::NumberOfProcesses)
        .def("allRanksAlive", [](MessagePasser& self, int seconds_to_wait) {
            auto status = self.NonBlockingBarrier();
            return status.waitFor(double(seconds_to_wait));
        });
}
