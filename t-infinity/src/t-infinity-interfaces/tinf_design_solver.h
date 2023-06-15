#pragma once
#ifndef TINF_DESIGN_SOLVER_H
#define TINF_DESIGN_SOLVER_H

#include <stdlib.h>

#include "tinf_enum_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*tinf_solver_create_f)(void** solver, void* problem_definition, void* tinf_mesh, void* iris_comm, int32_t* err);
typedef void (*tinf_solver_destroy_f)(void** solver, int32_t* err);

typedef void (*tinf_solver_get_nodal_input_names_f)(void* solver, size_t* n_inputs, char** names, int32_t* err);
typedef void (*tinf_solver_delete_input_names_f)(void* solver, size_t n_inputs, char** names, int32_t* err);

// we should probably make the solver return an integer
// error flag for each function to match the mesh interface

void tinf_solver_create(void** solver, void* problem_definition, void* tinf_mesh, void* iris_comm, int32_t* err);
void tinf_solver_destroy(void** solver, int32_t* err);

// independent inputs (Reynolds number, prandtl number, reference temp, freestream conditions, etc)
//void tinf_solver_set_problem_definition(void* solver, void* problem_definition)

// nonlinear solver
void tinf_solver_step_pre(void* solver, int32_t* err); // shuffle backplanes, etc.
void tinf_solver_step_solver(void* solver, int32_t* err); // update state
void tinf_solver_step_post(void* solver, int32_t* err); //compute non-state outputs, etc.
void tinf_solver_step_apply_nonlinear(void* solver, int32_t* err); // evaluate the residual given some input and output do. not update the output

// linear solve - how to extend these to solving with vectors instead of only 1 scalar? e.g., dR/d{x,y,z} instead of dR/dx, dR/dy, dR/dz
//              - does this work beyond steady problems?
void tinf_solver_solve_linear(void* solver, int32_t transposed, size_t n_inputs, char* d_input_name, char* input_name, char* output_name, int32_t* err);
// solve dR/d{d_input_name} * {output_name} = {input_name}
void tinf_solver_apply_linear(void* solver, int32_t transposed, size_t n_inputs, char* d_input_name, char* input_name, char* output_name, int32_t* err);
// matvec dR/d{d_input_name} * {input_name} = {output_name}

// overset
void tinf_solver_get_solution_in_cell(void* solver, double x, double y, double z, int64_t cell_id, double* solution, int32_t* err);
void tinf_solver_freeze_nodes(void* solver, const int* freeze_node_ids, int64_t number_of_frozen_nodes, int32_t* err);

// inputs and outputs at nodes
//int32_t tinf_solver_number_of_input_scalars_at_nodes(void* solver);
//int32_t tinf_solver_number_of_output_scalars_at_nodes(void* solver);
void tinf_solver_get_nodal_input_names(void* solver, size_t* n_inputs, char** names, int32_t* err);
void tinf_solver_get_nodal_output_names(void* solver, size_t n_outputs, char** names, int32_t* err);
void tinf_solver_set_input_at_node(void* solver, int64_t node_id, const double value, char* scalar_name, int32_t* err); // replace scalar_name with scalar index?
void tinf_solver_set_output_at_node(void* solver, int64_t node_id, const double value, char* scalar_name, int32_t* err);
double tinf_solver_get_output_at_node(void* solver, int64_t node_id, char* scalar_name, int32_t* err);

// Alternate: independent inputs for scalars that need derivatives
int32_t tinf_solver_number_of_input_independent_scalars(void* solver);
void tinf_solver_get_independent_input_scalar_name(void* solver, int64_t scalar_index, char* scalar_name, int32_t* err);
void tinf_solver_set_independent_input_scalar(void* solver, const double input, char* scalar_name, int32_t* err);
double tinf_solver_apply_linear_independent_input(void* solver, char* d_input_name, char* input_name, int32_t* err);
// matvec  dR/d{d_input_name} * {input_name} = double

// TODO: Global outputs like integrated lift, RMS residuals

//void tinf_solver_update_node_locations(void* solver, const double* xyz, int64_t number_of_nodes);
// update locations not needed if input list includes x,y,z

#ifdef __cplusplus
}
#endif

#endif
