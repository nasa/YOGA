#ifndef DISTANCE_CALCULATOR_H
#define DISTANCE_CALCULATOR_H

#include <t-infinity/Mangle.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

// original single function call
int32_t calculate_distance_to_nodes(
    int fortran_comm, void* tinf_mesh, int64_t* wall_tags, int64_t number_of_wall_tags, double* node_distance);

// object calls that change state
int32_t mangle(tinf_distance_create)(void** handle, int fortran_comm, void* tinf_mesh);
int32_t mangle(tinf_distance_destroy)(void** handle);
int32_t mangle(tinf_distance_calculate)(void* handle, int64_t* wall_tags, int64_t num_tags);

// object accessor calls that fill output arrays for each node in the mesh.
int32_t mangle(tinf_distance_distance_to_surface)(void* self, double* distance);
int32_t mangle(tinf_distance_global_cell_id_of_surface_element)(void* self, int64_t* gcid);
int32_t mangle(tinf_distance_cell_owner_of_surface_cell)(void* self, int64_t* owner);

#ifdef __cplusplus
}
#endif

#endif
