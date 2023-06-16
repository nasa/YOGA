#pragma once
#ifndef TINF_SOLVER_H
#define TINF_SOLVER_H

#include <stdlib.h>

#include "tinf_solution.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef int32_t (*tinf_solver_create_f)(void** solver,
                                        void* problem_definition,
                                        void* tinf_mesh, void* iris_comm);
typedef int32_t (*tinf_solver_destroy_f)(void** solver);
typedef int32_t (*tinf_solver_get_version_f)(void* solver,
                                             const char** version);
typedef int32_t (*tinf_solver_freeze_nodes_f)(void* solver,
                                              const int64_t* freeze_node_ids,
                                              int64_t number_of_frozen_nodes);
typedef int32_t (*tinf_solver_step_pre_f)(void* solver);
typedef int32_t (*tinf_solver_step_f)(void* solver, double seconds_to_advance);
typedef int32_t (*tinf_solver_step_post_f)(void* solver);

/**
 * Constructor.
 *
 * @param solver  Opaque solver pointer
 * @param problem_definition  Context defining the problem
 * @param tinf_mesh  Input opaque mesh pointer
 * @param iris_comm  Iris communications opaque pointer
 * @returns  Error code
 */
int32_t tinf_solver_create(void** solver, void* problem_definition,
                           void* tinf_mesh, void* iris_comm);
/**
 * Destructor.
 *
 * @param solver  Opaque solver pointer
 * @returns  Error code
 */
int32_t tinf_solver_destroy(void** solver);

/**
 * Return the version description of the solver.
 *
 * @param solver  Opaque solver pointer
 * @param version  description of solver version, e.g., "13.5-<git sha>" (output)
 * @returns  Error code
 */
int32_t tinf_solver_get_version(void* solver, const char** version);

/**
 * Define nodes at which solution is not to be updated (blanked).
 *
 * @param solver  Opaque solver pointer
 * @param freeze_node_ids  List of node indices to freeze (length @p number_of_frozen_nodes)
 * @param number_of_frozen_nodes  Number of node indices to freeze
 * @returns  Error code
 */
int32_t tinf_solver_freeze_nodes(void* solver,
                                 const int64_t* freeze_node_ids,
                                 int64_t number_of_frozen_nodes);

/**
 * Perform nonlinear step preprocessing (shuffle backplanes, etc.) prior
 * to @ref tinf_solver_step
 *
 * @param solver  Opaque solver pointer
 * @returns  Error code
 */
int32_t tinf_solver_step_pre(void* solver);

/**
 * Perform nonlinear step preprocessing (update state)
 *
 * @param solver  Opaque solver pointer
 * @param seconds_to_advance  Duration of physical time to advance
 * @returns  Error code
 */
int32_t tinf_solver_step(void* solver, double seconds_to_advance);

/**
 * Perform nonlinear step post-processing (compute nonstate outputs, etc.)
 * following a call to @ref tinf_solver_step
 *
 * @param solver  Opaque solver pointer
 * @returns  Error code
 */
int32_t tinf_solver_step_post(void* solver);

#ifdef __cplusplus
}
#endif

#endif
