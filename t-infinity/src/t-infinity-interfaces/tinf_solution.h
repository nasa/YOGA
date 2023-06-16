#pragma once
#ifndef TINF_SOLUTION_H
#define TINF_SOLUTION_H

#include <stdlib.h>

#include "tinf_enum_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*tinf_solution_get_nodal_input_names_f)(void* const solution,
                                     int64_t* const n_inputs,
                                     const char** names[],
                                     const enum TINF_DATA_TYPE* datatype[],
                                     const int32_t* rank[],
                                     const size_t** dims[TINF_DATA_MAX_RANK]);
typedef int32_t (*tinf_solution_set_inputs_at_nodes_f)(void* const solution,
                                     const enum TINF_DATA_TYPE data_type,
                                     const int64_t start_node,
                                     const int64_t node_count,
                                     const int64_t val_count,
                                     const char* const name[],
                                     const int32_t rank,
                                     const size_t dims[TINF_DATA_MAX_RANK],
                                     const void* const values);
typedef int32_t (*tinf_solution_get_nodal_output_names_f)(void* const solution,
                                     int64_t* const n_outputs,
                                     const char** names[],
                                     const enum TINF_DATA_TYPE* datatype[]);
typedef int32_t (*tinf_solution_get_outputs_at_nodes_f)(void* const solution,
                                     const enum TINF_DATA_TYPE data_type,
                                     const int64_t start_node,
                                     const int64_t node_count,
                                     const int64_t value_count,
                                     const char* names[],
                                     void* const values);
typedef int32_t (*tinf_solution_get_outputs_in_cell_f)(void* const solution,
                                     const enum TINF_DATA_TYPE xyz_type,
                                     const enum TINF_DATA_TYPE data_type,
                                     const void* const x, const void* const y,
                                     const void* const z, const int64_t cell_id,
                                     const int64_t value_count,
                                     const char* names[],
                                     void* const values);

/**
 * Return the number and list of variable names of possible inputs at nodes
 *
 * @param solution  Opaque solution pointer
 * @param n_inputs  Number of inputs
 * @param names  Names of the inputs
 * @param datatype   Types of the data to set at each node
 * @param rank   Ranks of the data to set at each node
 * @param dims   Data dimensions for each rank (length @p rank)
 * @returns Error code
 */
int32_t tinf_solution_get_nodal_input_names(void* const solution,
                                       int64_t* const n_inputs,
                                       const char** names[],
                                       const enum TINF_DATA_TYPE* datatype[],
                                       const int32_t* rank[],
                                       const size_t** dims[TINF_DATA_MAX_RANK]);

/**
 * Set the named inputs of the solution to varying values over a range of nodes.
 *
 * @param solution  Opaque solution pointer
 * @param data_type  Data type (@ref TINF_DATA_TYPE) of output values
 * @param start_node  Starting target mesh node ID
 * @param node_count  Number of target nodes to follow @p start_node
 * @param val_count  Number of target values to set at each node
 * @param name   Array of @p val_count value names (NULL terminated) to set
 *               at each node
 * @param rank   Rank of the data to set at each node
 *               (All @p val_count values have the same rank)
 * @param dims   Data dimension for each rank (length @p rank)
 *               (All @p val_count values have the same dimensions for each @p rank)
 * @param values  Pointer to storage for @p data_type input values to set at
 *                each node (length @p node_count * @p dims[0]
 *                                         [* @p dims[1] ... * @p dims[rank-1])
 * @returns  Error code
 */
int32_t tinf_solution_set_inputs_at_nodes(void* const solution,
                                       const enum TINF_DATA_TYPE data_type,
                                       const int64_t start_node,
                                       const int64_t node_count,
                                       const int64_t val_count,
                                       const char* const name[],
                                       const int32_t rank,
                                       const size_t dims[TINF_DATA_MAX_RANK],
                                       const void* const values);

/**
 * Return the number and list of variable names provided as output from the
 * solution.
 *
 * @param solution  Opaque solution pointer
 * @param n_outputs  Number of outputs
 * @param names  Array of @p n_outputs names (NULL terminated) to set
 *               at each node
 * @param datatype  Types of the data to get at each node (length @p n_outputs)
 * @returns Error code
 */
int32_t tinf_solution_get_nodal_output_names(void* const solution,
                                         int64_t* const n_outputs,
                                         const char** names[],
                                         const enum TINF_DATA_TYPE* datatype[]);

/**
 * Get the named outputs of the solution over a range of nodes.
 *
 * @param solution  Opaque solution pointer
 * @param data_type  Data type of output values
 * @param start_node  Starting target mesh node ID
 * @param node_count  Number of target nodes to follow @p start_node
 * @param val_count  Number of target values to get at each node
 * @param names  Array of @p val_count names (NULL terminated) to get
 *               at each node
 * @param values  Pointer to storage for @p data_type output values to get at
 *                each node (length @p node_count * @p val_count)
 * @returns err  Error code
 */
int32_t tinf_solution_get_outputs_at_nodes(void* const solution,
                                     const enum TINF_DATA_TYPE data_type,
                                     const int64_t start_node,
                                     const int64_t node_count,
                                     const int64_t value_count,
                                     const char* names[],
                                     void* const values);

/**
 * Get solution at a location in a cell. (Overset, etc.)
 *
 * @param solution  Opaque solution pointer
 * @param xyz_type  Data type of target location coordinates
 * @param data_type  Data type of output values
 * @param x, y, z  Pointer to storage for target location coordinates of
 *                 type @p xyz_type
 * @param cell_id  Target cell for solution
 * @param val_count  Number of target values to get at each node
 * @param names  Array of @p val_count names (NULL terminated) to get
 *               at each node
 * @param values  Pointer to storage for @p data_type output values to get at
 *                the target location (length @p val_count)
 * @returns Error code
 */
int32_t tinf_solution_get_outputs_in_cell(void* const solution,
                                    const enum TINF_DATA_TYPE xyz_type,
                                    const enum TINF_DATA_TYPE data_type,
                                    const void* const x, const void* const y,
                                    const void* const z, const int64_t cell_id,
                                    const int64_t value_count,
                                    const char* names[],
                                    void* const values);

#ifdef __cplusplus
}
#endif

#endif
