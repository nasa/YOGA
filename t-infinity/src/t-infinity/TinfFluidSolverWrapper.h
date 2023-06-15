#pragma once
#include <FluidSolverInterface.h>
#include "Tinf_FluidSolver.h"

class TinfFluidSolverWrapper : public inf::FluidSolverInterface {
  public:
    TinfFluidSolverWrapper(std::shared_ptr<inf::MeshInterface> mesh, MPI_Comm comm) {
        solver_instance = tinf_fluid_solver_create((void*)mesh.get(), comm);
    }
    ~TinfFluidSolverWrapper() { tinf_fluid_solver_destroy(solver_instance); }
    void solve() override { tinf_fluid_solver_solve(solver_instance); }
    void freezeNodes(const std::vector<int>& local_node_ids) override {
        tinf_fluid_solver_freeze_nodes(
            solver_instance, local_node_ids.data(), local_node_ids.size());
    }

    void solutionInCell(
        double x, double y, double z, int cell_id, double* solution) const override {
        tinf_fluid_solver_get_solution_in_cell(solver_instance, x, y, z, cell_id, solution);
    }

    void setSolutionAtNode(int node_id, const double* solution) override {
        tinf_fluid_solver_set_solution_at_node(solver_instance, node_id, solution);
    }

    void updateNodeLocations(const std::vector<std::array<double, 3>>& node_coordinates) override {
        tinf_fluid_solver_update_node_locations(
            solver_instance, (double*)node_coordinates.data(), node_coordinates.size());
    }

    std::vector<std::string> listNodalScalars() const override {
        std::vector<std::string> scalar_names(
            (size_t)tinf_fluid_solver_number_of_scalars_at_nodes(solver_instance));
        for (int i = 0; i < scalar_names.size(); i++) {
            char* scalar_name;
            tinf_fluid_solver_get_scalar_name(solver_instance, i, scalar_name);
            scalar_names.push_back(scalar_name);
        }
        return scalar_names;
    }

    double getScalarAtNode(const std::string& name, int node_id) const override {
        return tinf_fluid_solver_get_scalar_at_node(solver_instance, name.c_str(), node_id);
    }

    std::vector<std::string> expectsSolutionAs() const override {
        throw std::logic_error("TinfFluidSolverWrapper::expectsSolutionAs is not implemented");
    }

  private:
    void* solver_instance;
};
