#pragma once
#ifndef TINF_DISTANCE_CALCULATOR_H
#define TINF_DISTANCE_CALCULATOR_H

#include <stdlib.h>

#include "tinf_distance_to_boundary.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef int32_t (*tinf_distance_calculator_create_f)(void** dcalc,
                                                         void* problem,
                                                         void* mesh,
                                                         void* comm);

    typedef int32_t (*tinf_distance_calculator_destroy_f)(void** dcalc);


    /**
     * Create a new distance calculator for a mesh.
     *
     * @param dcalc  The created distance calculator
     * @param problem  A problem object
     * @param mesh  A mesh object
     * @param comm  A communications object
     *
     * @returns  Error code
     **/
    int32_t tinf_distance_calculator_create(void** dcalc, void* problem,
                                            void* mesh, void* comm);

    /**
     * Destroy a distance calculator.
     *
     * @param dcalc  Target distance calculator object
     *
     * @returns  Error code
     **/
    int32_t tinf_distance_calculator_destroy(void** dcalc);

#ifdef __cplusplus
}
#endif

#endif
