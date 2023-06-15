#pragma once
#ifndef TINF_INPUT_FILE_H
#define TINF_INPUT_FILE_H

#include <stdlib.h>

#include "tinf_problem.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef void (*tinf_input_file_create_f)(void** problem,
                                             const char* filename,
                                             size_t namelen, void* comm,
                                             int32_t* err);
    typedef void (*tinf_input_file_destroy_f)(void** problem, int32_t* err);


    /**
     * Create an problem description based on an input file and implements
     * the problem interface.
     *
     * @param problem  Opaque problem pointer
     * @param filename  A file name that contains the problem description
     *                  (length @p namelen)
     * @param namelen  The lenght of the filename
     * @param comm  Opaque Iris pointer
     * @returns  Error code
     **/
    int32_t tinf_input_file_create(void** problem, const char* filename,
                                   size_t namelen, void* comm);

    /**
     * Destroy a problem description.
     *
     * @param problem  Opaque problem pointer
     * @returns  Error code
     **/
    int32_t tinf_input_file_destroy(void** problem);

#ifdef __cplusplus
}
#endif

#endif
