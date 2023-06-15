#include "FluidSolverTools.h"
#include "VectorFieldAdapter.h"
#include "Snap.h"
#include <parfait/VectorTools.h>
#include <Tracer.h>

std::shared_ptr<inf::FieldInterface> inf::extractScalarsAsNodeField(
    const inf::FluidSolverInterface& solver,
    const inf::MeshInterface& mesh,
    const std::string& field_name,
    const std::vector<std::string>& scalar_names) {
    Tracer::begin("extractScalarsAsNodeField");
    int nscalars = scalar_names.size();
    std::vector<double> scalars(nscalars * mesh.nodeCount());
#pragma omp parallel for
    for (int node_id = 0; node_id < mesh.nodeCount(); node_id++)
        for (int j = 0; j < nscalars; j++)
            scalars[node_id * nscalars + j] = solver.getScalarAtNode(scalar_names[j], node_id);
    auto field = std::make_shared<inf::VectorFieldAdapter>(
        field_name, FieldAttributes::Node(), nscalars, scalars);
    Tracer::end("extractScalarsAsNodeField");
    return field;
}
std::shared_ptr<inf::FieldInterface> inf::extractScalarsAsCellField(
    const inf::FluidSolverInterface& solver,
    const inf::MeshInterface& mesh,
    const std::string& field_name,
    const std::vector<std::string>& scalar_names) {
    Tracer::begin("extractScalarsAsCelLField");
    int nscalars = scalar_names.size();
    std::vector<double> scalars(nscalars * mesh.cellCount());
#pragma omp parallel for
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++)
        for (int j = 0; j < nscalars; j++)
            scalars[cell_id * nscalars + j] = solver.getScalarAtCell(scalar_names[j], cell_id);
    auto field = std::make_shared<inf::VectorFieldAdapter>(
        field_name, FieldAttributes::Cell(), nscalars, scalars);
    Tracer::end("extractScalarsAsCelLField");
    return field;
}

void inf::writeSolverFieldsToSnap(std::string file_name,
                                  std::shared_ptr<MeshInterface> mesh,
                                  std::shared_ptr<FluidSolverInterface> solver,
                                  std::vector<std::string> fields,
                                  MessagePasser mp) {
    Tracer::begin("writeSolverFieldToSnap");
    auto snap = inf::Snap(mp.getCommunicator());
    snap.addMeshTopologies(*mesh);
    auto available_node_fields = solver->listScalarsAtNodes();
    auto available_cell_fields = solver->listScalarsAtCells();
    for (const auto& s : fields) {
        if (Parfait::VectorTools::isIn(available_node_fields, s)) {
            snap.add(extractScalarsAsNodeField(*solver, *mesh, s, {s}));
        } else if (Parfait::VectorTools::isIn(available_cell_fields, s)) {
            snap.add(extractScalarsAsCellField(*solver, *mesh, s, {s}));
        } else {
            PARFAIT_THROW("Solver does not have field: " + s);
        }
    }
    snap.writeFile(std::move(file_name));
    Tracer::end("writeSolverFieldToSnap");
}

void inf::writeSolutionToSnap(std::string file_name,
                              std::shared_ptr<MeshInterface> mesh,
                              std::shared_ptr<inf::FluidSolverInterface> solver,
                              MessagePasser mp) {
    writeSolverFieldsToSnap(file_name, mesh, solver, solver->expectsSolutionAs(), mp);
}
void inf::setSolutionFromSnap(std::string file_name,
                              std::shared_ptr<MeshInterface> mesh,
                              std::shared_ptr<inf::FluidSolverInterface> solver,
                              MessagePasser mp) {
    Tracer::begin("setSolutionFromSnap");
    auto solution_field_names = solver->expectsSolutionAs();
    auto snap = inf::Snap(mp.getCommunicator());
    snap.addMeshTopologies(*mesh);
    snap.load(file_name);
    std::vector<std::shared_ptr<FieldInterface>> fields;
    for (const auto& s : solution_field_names) fields.push_back(snap.retrieve(s));
    setFluidSolverSolution(fields, solver);
    Tracer::end("setSolutionFromSnap");
}
void inf::setFluidSolverSolution(
    const std::vector<std::shared_ptr<FieldInterface>>& solution_fields,
    std::shared_ptr<FluidSolverInterface> solver) {
    Tracer::begin("setFluidSolverSolution");
    auto solution_field_names = solver->expectsSolutionAs();
    auto available_node_fields = solver->listScalarsAtNodes();
    auto available_cell_fields = solver->listScalarsAtCells();

    // Determine if solution is as nodes or cells
    int node_field_count = 0;
    int cell_field_count = 0;
    for (const auto& s : solution_field_names) {
        if (Parfait::VectorTools::isIn(available_node_fields, s)) node_field_count++;
        if (Parfait::VectorTools::isIn(available_cell_fields, s)) cell_field_count++;
    }
    PARFAIT_ASSERT(node_field_count == 0 or cell_field_count == 0,
                   "mixed node/cell solution not supported");

    std::vector<double> solution(solution_field_names.size());
    if (node_field_count > 0) {
        int node_count = solution_fields.front()->size();
#pragma omp parallel for
        for (int node_id = 0; node_id < node_count; node_id++) {
            for (size_t i = 0; i < solution_field_names.size(); i++) {
                solution_fields[i]->value(node_id, &solution[i]);
            }
            solver->setSolutionAtNode(node_id, solution.data());
        }
    }
    if (cell_field_count > 0) {
        int cell_count = solution_fields.front()->size();
#pragma omp parallel for
        for (int cell_id = 0; cell_id < cell_count; cell_id++) {
            for (size_t i = 0; i < solution_field_names.size(); i++) {
                solution_fields[i]->value(cell_id, &solution[i]);
            }
            solver->setSolutionAtCell(cell_id, solution.data());
        }
    }
    Tracer::end("setFluidSolverSolution");
}
