#pragma once
#ifndef TINF_FLUID_SOLVER_H
#define TINF_FLUID_SOLVER_H

#include <stdlib.h>
#include <mpi.h>


#ifdef __cplusplus
extern "C" {
#endif

// we should probably make the solver return an integer
// error flag for each function to match the mesh interface

void* tinf_fluid_solver_create(void* tinf_mesh, MPI_Comm comm);
void tinf_fluid_solver_destroy(void* solver);

void tinf_fluid_solver_solve(void* solver);
// probably want iterate, step setup, step post for time accurate
// nonlinear vs apply/solve linear, apply (internal matrix/jacobian) to input vector
void tinf_fluid_solver_get_solution_in_cell(void* solver, double x,double y,double z,int cell_id, double* solution);
void tinf_fluid_solver_set_solution_at_node(void* solver, int node_id, const double* solution);
void tinf_fluid_solver_update_node_locations(void* solver, const double* xyz, size_t number_of_nodes);
void tinf_fluid_solver_freeze_nodes(void* solver, const int* freeze_node_ids, size_t number_of_frozen_nodes);
double tinf_fluid_solver_get_scalar_at_node(void* solver, const char* name, int node_id);
int tinf_fluid_solver_number_of_scalars_at_nodes(void* solver);
void tinf_fluid_solver_get_scalar_name(void* solver, int scalar_index, char* scalar_name);

#ifdef __cplusplus
}
#endif

#endif
