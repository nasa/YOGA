#include <RingAssertions.h>
#include <t-infinity/Snap.h>
#include <t-infinity/FluidSolverTools.h>
#include <parfait/VectorTools.h>
#include "MockMeshes.h"

using namespace inf;

class MockFluidSolver : public FluidSolverInterface {
  public:
    explicit MockFluidSolver(const MeshInterface& mesh, bool is_node_solver = true)
        : node_was_set(mesh.nodeCount(), false), cell_was_set(mesh.cellCount(), false), node_solver(is_node_solver) {}
    bool solve(const std::string& per_solve_settings) override { return false; }
    void step(const std::string& per_step_settings) override {}
    void freezeNodes(const std::vector<int>& local_node_ids) override {}
    void freezeCells(const std::vector<int>& local_cell_ids) override {}
    void setSolutionAtNode(int node_id, const double* solution) override { node_was_set[node_id] = true; }
    void setSolutionAtCell(int cell_id, const double* solution) override { cell_was_set[cell_id] = true; }
    std::vector<std::string> listScalarsAtNodes() const override { return {"u", "v"}; }
    std::vector<std::string> listScalarsAtCells() const override { return {"pikachu"}; }
    double getScalarAtNode(const std::string& name, int node_id) const override {
        PARFAIT_ASSERT(Parfait::VectorTools::isIn(listScalarsAtNodes(), name), "not a node field: " + name);
        return 0;
    }
    double getScalarAtCell(const std::string& name, int cell_id) const override {
        PARFAIT_ASSERT(Parfait::VectorTools::isIn(listScalarsAtCells(), name), "not a cell field: " + name);
        return 0;
    }
    void solutionInCell(double x, double y, double z, int cell_id, double* solution) const override {}
    std::vector<std::string> expectsSolutionAs() const override {
        return node_solver ? std::vector<std::string>{"u", "v"} : std::vector<std::string>{"pikachu"};
    }

    std::vector<bool> node_was_set;
    std::vector<bool> cell_was_set;
    bool node_solver;
};

TEST_CASE("Can write FluidSolverFields to snap") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) return;
    auto mesh = mock::getSingleHexMeshWithFaces();
    auto snap = Snap(mp.getCommunicator());
    snap.addMeshTopologies(*mesh);
    std::string snap_filename = "solve_test_fields.snap";

    for (const std::string& solver_type : {FieldAttributes::Node(), FieldAttributes::Cell()}) {
        auto solver = std::make_shared<MockFluidSolver>(*mesh, solver_type == FieldAttributes::Node());
        SECTION("writeSolutionToSnap - " + solver_type) {
            writeSolutionToSnap(snap_filename, mesh, solver, mp);
            snap.load(snap_filename);
            REQUIRE(snap.availableFields() == solver->expectsSolutionAs());
        }
        SECTION("setSolutionFromSnap - " + solver_type) {
            writeSolutionToSnap(snap_filename, mesh, solver, mp);
            for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
                REQUIRE_FALSE(solver->node_was_set[node_id]);
                REQUIRE_FALSE(solver->cell_was_set[node_id]);
            }
            setSolutionFromSnap(snap_filename, mesh, solver, mp);
            if (solver->node_solver) {
                for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
                    REQUIRE(solver->node_was_set[node_id]);
                }
                for (int cell_id = 0; cell_id < mesh->cellCount(); ++cell_id) {
                    REQUIRE_FALSE(solver->cell_was_set[cell_id]);
                }
            } else {
                for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
                    REQUIRE_FALSE(solver->node_was_set[node_id]);
                }
                for (int cell_id = 0; cell_id < mesh->cellCount(); ++cell_id) {
                    REQUIRE(solver->cell_was_set[cell_id]);
                }
            }
        }
        SECTION("writeSolverFieldsToSnap - " + solver_type) {
            REQUIRE_THROWS(writeSolverFieldsToSnap(snap_filename, mesh, solver, {"unknown-field-name"}, mp));
            std::vector<std::string> solver_fields = {"pikachu", "u"};
            writeSolverFieldsToSnap(snap_filename, mesh, solver, solver_fields, mp);
            snap.load(snap_filename);
            REQUIRE(solver_fields == snap.availableFields());
        }
    }
    REQUIRE(0 == remove(snap_filename.c_str()));
}