#pragma once

#include <utility>
#include "FluidSolverInterface.h"
#include "MeshHelpers.h"
#include "GlobalToLocal.h"

namespace inf {
class FluidSolver2DWrapper : public inf::FluidSolverInterface {
  public:
    using Mapping = std::vector<std::pair<int, int>>;
    FluidSolver2DWrapper(std::shared_ptr<inf::FluidSolverInterface> solver,
                         Mapping local_node_mapping)
        : solver(std::move(solver)), node_offsets(std::move(local_node_mapping)) {}

    static Mapping calcTwodOffsetIds(long offset,
                                     const std::vector<long>& local_to_global_2d_source,
                                     const std::map<long, int>& global_to_local_3d_target) {
        Mapping offset_ids(local_to_global_2d_source.size());
        for (size_t local_id = 0; local_id < local_to_global_2d_source.size(); ++local_id) {
            auto gid = local_to_global_2d_source.at(local_id);
            offset_ids[local_id].first = global_to_local_3d_target.at(gid);
            offset_ids[local_id].second = global_to_local_3d_target.at(gid + offset);
        }
        return offset_ids;
    }

    static Mapping calcTwodOffsetIds(const MessagePasser& mp,
                                     const MeshInterface& source_2d_mesh,
                                     const MeshInterface& target_3d_mesh) {
        PARFAIT_ASSERT(inf::is2D(mp, source_2d_mesh), "Requested twod wrapper, but mesh is not 2D");
        auto offset =
            inf::globalNodeCount(mp, target_3d_mesh) - inf::globalNodeCount(mp, source_2d_mesh);
        auto source_l2g = inf::LocalToGlobal::buildNode(source_2d_mesh);
        auto target_g2l = inf::GlobalToLocal::buildNode(target_3d_mesh);
        return inf::FluidSolver2DWrapper::calcTwodOffsetIds(offset, source_l2g, target_g2l);
    }

    bool solve(const std::string& per_solve_settings) override {
        return solver->solve(per_solve_settings);
    }
    void step(const std::string& per_step_settings) override { solver->step(per_step_settings); }
    void freezeNodes(const std::vector<int>& local_node_ids) override {
        std::vector<int> nodes_with_offsets;
        nodes_with_offsets.reserve(2 * local_node_ids.size());
        for (const auto& node_id : local_node_ids) {
            nodes_with_offsets.push_back(node_offsets.at(node_id).first);
            nodes_with_offsets.push_back(node_offsets.at(node_id).second);
        }
        solver->freezeNodes(nodes_with_offsets);
    }
    void freezeCells(const std::vector<int>& local_cell_ids) override {
        solver->freezeCells(local_cell_ids);
    }
    void solutionInCell(
        double x, double y, double z, int cell_id, double* solution) const override {
        solver->solutionInCell(x, y, z, cell_id, solution);
    }
    void setSolutionAtNode(int node_id, const double* solution) override {
        solver->setSolutionAtNode(node_offsets.at(node_id).first, solution);
        solver->setSolutionAtNode(node_offsets.at(node_id).second, solution);
    }
    void setSolutionAtCell(int cell_id, const double* solution) override {
        solver->setSolutionAtCell(cell_id, solution);
    }
    std::vector<std::string> listScalarsAtNodes() const override {
        return solver->listScalarsAtNodes();
    }
    std::vector<std::string> listScalarsAtCells() const override {
        return solver->listScalarsAtCells();
    }
    double getScalarAtNode(const std::string& name, int node_id) const override {
        return solver->getScalarAtNode(name, node_offsets.at(node_id).first);
    }
    double getScalarAtCell(const std::string& name, int cell_id) const override {
        return solver->getScalarAtCell(name, cell_id);
    }
    std::vector<std::string> expectsSolutionAs() const override {
        return solver->expectsSolutionAs();
    }

  private:
    std::shared_ptr<inf::FluidSolverInterface> solver;
    Mapping node_offsets;
};
}
