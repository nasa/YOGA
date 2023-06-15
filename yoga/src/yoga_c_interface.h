#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void* yoga_create_instance(int comm,
                           int (*getNumberOfNodes)(),
                           int (*getNumberOfCells)(),
                           int (*getNumberOfNodesInCell)(int),
                           void (*getNodesInCell)(int, int*),
                           long (*getGlobalNodeId)(int),
                           int (*getComponentId)(int),
                           void (*getPoint)(int, double*),
                           int (*isGhostNode)(int),
                           int (*getNumberOfBoundaryFaces)(),
                           int (*numberOfNodesInBoundaryFace)(int),
                           void (*getNodesInFace)(int, int*),
                           int (*getBoundaryCondition)(int),
                           int numberOfSolutionVars,
                           void (*getSolutionInCellAt)(int, double, double, double, double*),
                           void (*getSolutionAtNode)(int, double*));
void yoga_perform_domain_assembly(void* yoga_instance);
void yoga_update_receptor_solutions(void* yoga_instance);
void yoga_fill_grid_ids_for_nodes(void* yoga_instance,int* grid_ids);
int yoga_get_node_status(void* yoga_instance, int node_id);
void yoga_get_solution_for_receptor(void* yoga_instance, int node_id, double* Q);
void yoga_destroy_instance(void* yoga_instance);

int yoga_get_donor_count(void* yoga_instance, int node_id);
void yoga_fill_donor_local_ids(void* yoga_instance, int node_id, int* donor_ids);
void yoga_fill_donor_owning_ranks(void* yoga_instance, int node_id, int* owning_ranks);
void yoga_fill_donor_weights(void* yoga_instance, int node_id, double* weights);

void* yoga_create_linearized_donor_stencil(int n_donors, double* donor_xyzs, double* query_xyz);
void yoga_destroy_linearized_donor_stencil(void* stencil);
double yoga_partial_donor_x(void* stencil, int weight_index, int donor_index);
double yoga_partial_donor_y(void* stencil, int weight_index, int donor_index);
double yoga_partial_donor_z(void* stencil, int weight_index, int donor_index);
double yoga_partial_receptor_x(void* stencil, int weight_index);
double yoga_partial_receptor_y(void* stencil, int weight_index);
double yoga_partial_receptor_z(void* stencil, int weight_index);
#ifdef __cplusplus
}
#endif
