#pragma once
#include <mpi.h>
#include <array>
#include <memory>
#include <string>
#include <vector>
#include "PluginLoader.h"
namespace inf {
class StructuralSolverInterface {
  public:
    virtual std::vector<std::array<double, 3>> getNodes() = 0;

    // Iterate returns the structural deflections give the structural forces
    virtual std::vector<std::array<double, 3>> iterate(
        const std::vector<std::array<double, 3>>& structural_forces) = 0;
    virtual ~StructuralSolverInterface() = default;
};

class StructuralSolverFactory {
  public:
    virtual std::shared_ptr<StructuralSolverInterface> createStructuralSolver(
        MPI_Comm comm, std::string json) const = 0;
    virtual ~StructuralSolverFactory() = default;
};

inline auto getStructuralSolverFactory(std::string directory, std::string name) {
    return PluginLoader<StructuralSolverFactory>::loadPlugin(
        directory, name, "createStructuralSolverFactory");
}
}
