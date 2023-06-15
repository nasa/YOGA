#pragma once
#ifndef TINF_MESH_FILE_H
#define TINF_MESH_FILE_H

#include <stdlib.h>

#include "tinf_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef int32_t (*tinf_mesh_file_create_f)(void** mesh,
                                               const char* name,
                                               const char* style,
                                               const char* form,
                                               void* problem, void* comm);
    typedef int32_t (*tinf_mesh_file_destroy_f)(void** mesh);


    /**
     * Create a mesh from a file that implements the mesh interface.
     *
     * @param mesh  Opaque mesh pointer
     * @param name  The name of the file containing the mesh
     * @param nlen  Length of @p name
     * @param style  The style of the data layout in the file
     * @param slen  Length of @p style
     * @param form  The data format (i.e. 'binary')
     * @param flen  Length of @p form
     * @param problem  A problem description object
     * @param comm  A communications object
     * @returns  Error code
     **/
    int32_t tinf_mesh_file_create(void** mesh,
                                  const char* name,
                                  size_t nlen,
                                  const char* style,
                                  size_t slen,
                                  const char* form,
                                  size_t flen,
                                  void* problem, void* comm);

    /**
     * Destroy a mesh from file.
     *
     * @param mesh  Opaque mesh pointer
     * @returns  Error code
     **/
    int32_t tinf_mesh_file_destroy(void** mesh);

#ifdef __cplusplus
}
#endif

#endif
