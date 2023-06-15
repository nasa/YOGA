#pragma once

#include <MessagePasser/MessagePasser.h>
#include <t-infinity/FluidSolverInterface.h>

namespace inf {
void writeSolutionToSnap(std::string file_name,
                         std::shared_ptr<MeshInterface> mesh,
                         std::shared_ptr<FluidSolverInterface> solver,
                         MessagePasser mp);
void writeSolverFieldsToSnap(std::string file_name,
                             std::shared_ptr<MeshInterface> mesh,
                             std::shared_ptr<FluidSolverInterface> solver,
                             std::vector<std::string> fields,
                             MessagePasser mp);
void setSolutionFromSnap(std::string file_name,
                         std::shared_ptr<MeshInterface> mesh,
                         std::shared_ptr<FluidSolverInterface> solver,
                         MessagePasser mp);
void setFluidSolverSolution(const std::vector<std::shared_ptr<FieldInterface>>& solution_fields,
                            std::shared_ptr<FluidSolverInterface> solver);
std::shared_ptr<inf::FieldInterface> extractScalarsAsNodeField(
    const inf::FluidSolverInterface& solver,
    const inf::MeshInterface& mesh,
    const std::string& field_name,
    const std::vector<std::string>& scalar_names);
std::shared_ptr<inf::FieldInterface> extractScalarsAsCellField(
    const inf::FluidSolverInterface& solver,
    const inf::MeshInterface& mesh,
    const std::string& field_name,
    const std::vector<std::string>& scalar_names);
}
