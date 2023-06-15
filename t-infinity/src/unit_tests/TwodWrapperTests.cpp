#include <RingAssertions.h>
#include <parfait/Throw.h>
#include <t-infinity/FluidSolver2DWrapper.h>
#include <t-infinity/MeshExtruderInterface.h>
#include "MockMeshes.h"
#include <unordered_map>
#include <utility>

using namespace inf;

class SolverShunt : public FluidSolverInterface {
  public:
    bool solve(const std::string& per_solve_settings) override { return true; }
    void step(const std::string& per_step_settings) override {}
    void freezeNodes(const std::vector<int>& local_node_ids) override {}
    void freezeCells(const std::vector<int>& local_cell_ids) override {}
    void solutionInCell(double x, double y, double z, int cell_id, double* solution) const override {
        cell_ids_set.push_back(cell_id);
    }
    void setSolutionAtNode(int node_id, const double* solution) override { node_ids_set.push_back(node_id); }
    void setSolutionAtCell(int cell_id, const double* solution) override { cell_ids_set.push_back(cell_id); }
    std::vector<std::string> listScalarsAtNodes() const override { return {"dummy"}; }
    std::vector<std::string> listScalarsAtCells() const override { return {"dummy"}; }
    double getScalarAtNode(const std::string& name, int node_id) const override { return 1.0; }
    double getScalarAtCell(const std::string& name, int cell_id) const override { return 1.0; }
    std::vector<std::string> expectsSolutionAs() const override { return {"dummy"}; }
    mutable std::vector<int> node_ids_set;
    mutable std::vector<int> cell_ids_set;
};

TEST_CASE("Can adjust cell/node for twod mesh") {
    auto solver = std::make_shared<SolverShunt>();
    REQUIRE(solver->node_ids_set.empty());
    REQUIRE(solver->cell_ids_set.empty());

    std::vector<std::pair<int, int>> node_offsets = {{0, 3}, {1, 4}, {2, 7}};

    double dummy_solution = 0;
    auto twod_wrapper = FluidSolver2DWrapper(solver, node_offsets);
    twod_wrapper.setSolutionAtNode(2, &dummy_solution);
    REQUIRE(2 == solver->node_ids_set.at(0));
    REQUIRE(7 == solver->node_ids_set.at(1));
}

TEST_CASE("Can compute twod offsets from local-to-global mappings") {
    std::vector<long> l2g = {0, 1};
    std::map<long, int> g2l = {{0, 0}, {1, 1}, {10, 3}, {11, 2}};
    auto offsets = FluidSolver2DWrapper::calcTwodOffsetIds(10, l2g, g2l);
    REQUIRE(2 == offsets.size());
    REQUIRE(0 == offsets.at(0).first);
    REQUIRE(3 == offsets.at(0).second);
    REQUIRE(1 == offsets.at(1).first);
    REQUIRE(2 == offsets.at(1).second);
}

TEST_CASE("Can build 2D mapping from source/target mesh") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh_2d = TinfMesh(mock::oneTriangle(), 0);
    auto mesh_3d = TinfMesh(mock::onePrism(), 0);
    auto offsets = FluidSolver2DWrapper::calcTwodOffsetIds(mp, mesh_2d, mesh_3d);
    REQUIRE((int)offsets.size() == mesh_2d.nodeCount());
    for (int node_id = 0; node_id < mesh_2d.nodeCount(); ++node_id) {
        const auto& p = offsets[node_id];
        REQUIRE(p.first + 3 == p.second);
    }
}