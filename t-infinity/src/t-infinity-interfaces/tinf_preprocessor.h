#pragma once
#ifndef TINF_PREPROCESSOR_H
#define TINF_PREPROCESSOR_H

#include <stdlib.h>

#include "tinf_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef int32_t (*tinf_preprocessor_create_f)(void** mesh, void* problem,
                                                  void* comm);
    typedef int32_t (*tinf_preprocessor_destroy_f)(void** mesh);


    /**
     * Create a mesh preprocessor that implements the mesh interface.
     *
     * @param mesh  The generated mesh preprocessor object
     * @param problem  A problem description object
     * @param comm  A communications object
     * @returns  Error code
     **/
    int32_t tinf_preprocessor_create(void** mesh, void* problem, void* comm);

    /**
     * Destroy a mesh preprocessor.
     *
     * @param mesh  Target mesh preprocessor object
     * @returns  Error code
     **/
    int32_t tinf_preprocessor_destroy(void** mesh);

#ifdef __cplusplus
}
#endif

#endif
