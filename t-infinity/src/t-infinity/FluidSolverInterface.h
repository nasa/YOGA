#pragma once
#include <mpi.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include "FieldInterface.h"
#include "MeshInterface.h"
#include "PluginLoader.h"

namespace inf {

class FluidSolverInterface {
  public:
    virtual bool solve(const std::string& per_solve_settings) = 0;
    virtual void step(const std::string& per_step_settings) = 0;
    virtual void freezeNodes(const std::vector<int>& local_node_ids) = 0;
    virtual void freezeCells(const std::vector<int>& local_cell_ids) = 0;
    virtual void setSolutionAtNode(int node_id, const double* solution) = 0;
    virtual void setSolutionAtCell(int cell_id, const double* solution) = 0;
    virtual std::vector<std::string> listScalarsAtNodes() const = 0;
    virtual std::vector<std::string> listScalarsAtCells() const = 0;
    virtual double getScalarAtNode(const std::string& name, int node_id) const = 0;
    virtual double getScalarAtCell(const std::string& name, int cell_id) const = 0;
    virtual void solutionInCell(
        double x, double y, double z, int cell_id, double* solution) const = 0;
    virtual std::vector<std::string> expectsSolutionAs() const = 0;
    virtual ~FluidSolverInterface() = default;
};

class FluidSolverFactory {
  public:
    virtual std::shared_ptr<FluidSolverInterface> createFluidSolver(
        std::shared_ptr<MeshInterface> mesh, std::string json_config, MPI_Comm comm) const = 0;
    virtual ~FluidSolverFactory() = default;
};
inline auto getFluidSolverFactory(const std::string& directory, const std::string& name) {
    return PluginLoader<FluidSolverFactory>::loadPlugin(
        directory, name, "createFluidSolverFactory");
}
}
