#pragma once

#include "tinf_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

void* tinf_create_domain_assembler(void* mesh, int fortran_comm);
void tinf_destroy_domain_assembler(void* handle);

void tinf_perform_domain_assembly(void* handle);
void tinf_update_receptor_solutions(void* handle);
int tinf_get_node_status(void* handle, int node_id);
void tinf_get_solution_for_node(void* handle, int node_id, double* values);

#ifdef __cplusplus
}
#endif
