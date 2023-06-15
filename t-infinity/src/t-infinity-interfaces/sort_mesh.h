#ifndef SORT_MESH_H
#define SORT_MESH_H

#include "tinf_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t sort_mesh_create(void** sort_mesh, void* mesh, void* comm);

int32_t sort_mesh_destroy(void* sort_mesh);

int32_t sort_mesh_node_count(void* mesh, int64_t* count);

int32_t sort_mesh_resident_node_count(void* mesh, int64_t* count);

int32_t sort_mesh_element_count(void* mesh, int64_t* count);

int32_t sort_mesh_element_type_count(void* mesh, TINF_ELEMENT_TYPE type, int64_t* count);

int32_t sort_mesh_nodes_coordinate(void* mesh, int64_t start, int64_t cnt, double* x, double* y, double* z);
int32_t sort_mesh_element_type(void* mesh, int64_t element_id, TINF_ELEMENT_TYPE* type);
int32_t sort_mesh_element_nodes(void* mesh, int64_t element_id, int64_t* element_nodes);
int32_t sort_mesh_global_node_id(void* mesh, int64_t local_id, int64_t* global_id);
int32_t sort_mesh_global_element_id(void* mesh, int64_t local_id, int64_t* global_id);
int32_t sort_mesh_element_tag(void* mesh, int64_t element_id, int64_t* tag);

int32_t sort_mesh_element_owner(void* mesh, int64_t element_id, int64_t* partition);
int32_t sort_mesh_node_owner(void* mesh, int64_t node_id, int64_t* partition);
int32_t sort_mesh_partition_id(void* mesh, int64_t* partition);

#ifdef __cplusplus
}
#endif

#endif
