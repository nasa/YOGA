#pragma once
#include "../t-infinity-interfaces/tinf_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif
int32_t tinf_preprocess(void** tinf_mesh, void* mpi_comm, const char* name);
int32_t tinf_mesh_destroy(void** tinf_mesh);

#ifdef __cplusplus
}
#endif
