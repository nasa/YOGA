#pragma once
#ifndef TINF_DISTANCE_TO_NODES_H
#define TINF_DISTANCE_TO_NODES_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef int32_t (*tinf_distance_to_boundary_f)(void* dcalc,
                                                   int64_t* wall_tags,
                                                   int64_t n_wall_tags,
                                                   int64_t start, int64_t cnt,
                                                   double* node_distance);
    /**
     * Calculate the distance from points to the nearest boundary.
     *
     * @param dcalc  A distance calculator object
     * @param wall_tags  Candidate boundary tags to search
     * @param n_wall_tags  Number of candidate boundary tags
     * @param start  Starting node to retrieve (bias 1)
     * @param cnt  Number of nodes to retrieve
     * @param node_distance  Calculated distance to nearest node on
     *                       candidate boundary tags
     * @returns  Error code
     **/
    int32_t tinf_distance_to_boundary(void* dcalc,
                                      int64_t* wall_tags, int64_t n_wall_tags,
                                      int64_t start, int64_t cnt,
                                      double* node_distance);

#ifdef __cplusplus
}
#endif

#endif
