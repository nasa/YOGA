#pragma once
#ifndef TINF_SOLVER_SENSITIVITIES_H
#define TINF_SOLVER_SENSITIVITIES_H

#include <stdlib.h>

#include "tinf_solver.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*tinf_solver_apply_nonlinear_f)(void* solver_instance);
typedef int32_t (*tinf_solver_solve_linear_f)(void* solver_instance, int32_t transposed);
typedef int32_t (*tinf_solver_apply_linear_problem_input_f)(void* solver_instance,
                                                            int64_t n_variables,
                                                            char* variable_names[],
                                                            void* const values,
                                                            int64_t values_per_variable,
                                                            int32_t transposed);
typedef int32_t (*tinf_solver_apply_linear_nodal_input_f)(void* solver_instance,
                                                          char* variable_name,
                                                          int32_t transposed);

/**
 * Compute the residual
 *
 * @param solver_instance  Solver context
 * @returns Error code
 */
int32_t tinf_solver_apply_nonlinear(void* solver_instance);

/**
 * Perform linear solve of LHS x = RHS
 *
 * @param solver_instance  Solver context
 * @param transposed Whether or not the LHS is tranposed
 * @returns Error code
 */
int32_t tinf_solver_solve_linear(void* solver_instance, int32_t transposed);

/**
 * Perform matrix multiplication of d(Residual)/d(problem variable) * value = nodal_value
 * or d(Residual)/d(problem variable)^T * nodal_value = value
 *
 * @param solver_instance  Solver context
 * @param transposed Whether or not the Jacobian is transposed
 * @param n_variables Number of problem variables to linearize with respect to
 * @param variable_names Which variables to linearize with respect to
 * @param values linearization variables associated with problem variables.
 *               # of rows = n_variables
 *               # of columns = values_per_variables. (output in transposed mode)
 * @param values_per_variable how many values/vectors, x, to do matmult of
 * @returns Error code
 */
int32_t tinf_solver_apply_linear_problem_input(void* solver_instance,
                                               int64_t n_variables,
                                               char* variable_names[],
                                               void* const values,
                                               int64_t values_per_variable,
                                               int32_t transposed);

/**
 * Perform matrix multiplication of d(Residual)/d(nodal_input) * nodal_value_1 = nodal_value_2
 * or d(Residual)/d(nodal_input)^T * nodal_value_1 = value_value_2
 *
 * @param solver_instance  Solver context
 * @param transposed Whether or not the Jacobian is transposed
 * @param variable_name Which variable to linearize with respect to
 * @returns Error code
 */
int32_t tinf_solver_apply_linear_nodal_input(void* solver_instance,
                                             char* variable_name,
                                             int32_t transposed);

#ifdef __cplusplus
}
#endif

#endif
