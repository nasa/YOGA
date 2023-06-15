#pragma once
#ifndef TINF_PROBLEM_H
#define TINF_PROBLEM_H

#include <stdlib.h>

#include "tinf_enum_definitions.h"

#ifdef __cplusplus
#define _BOOL_ bool
#else
#define _BOOL_ _Bool
#endif

#ifdef __cplusplus
extern "C" {
#endif
typedef int32_t (*tinf_problem_create_f)(void** problem, void* iris);
typedef int32_t (*tinf_problem_destroy_f)(void** problem);
typedef _BOOL_ (*tinf_problem_defined_f)(void* problem,
                                         const char* key,
                                         size_t keylen,
                                         enum TINF_DATA_TYPE* datatype,
                                         int32_t* rank,
                                         size_t dims[TINF_PROBLEM_MAX_RANK],
                                         int32_t* err);
typedef int32_t (*tinf_problem_value_f)(void* problem,
                                        const char* key,
                                        size_t keylen,
                                        const void* data,
                                        int32_t rank,
                                        const size_t dims[]);
typedef int32_t (*tinf_problem_set_value_f)(void* problem,
                                     const char* key,
                                     size_t keylen,
                                     const void* data,
                                     int32_t rank,
                                     const size_t dims[]);

/**
 * Constructor
 *
 * @param problem  Opaque problem pointer
 * @param iris iris context
 * @returns  Error code
 */
int32_t tinf_problem_create(void** problem, void* iris);

/**
 * Destructor
 *
 * @param problem  Opaque problem pointer
 * @returns  Error code
 */
int32_t tinf_problem_destroy(void** problem);

/**
 * Determine if the problem description defines a named parameter.
 *
 * @param problem  Opaque problem pointer
 * @param key  Requested parameter name (length @p keylen)
 * @param keylen  Length of the parameter name
 * @param datatype  Data type of parameter if defined.  If the parameter is not
 *                  defined the type is undefined.
 * @param rank  Rank of the parameter,  If the parameter is not defined the rank
 *              is undefined.
 * @param dims  Array of length for each dimension (length
 *              @p TINF_PROBLEM_MAX_RANK).  If the parameter is not defined
 *              this argument is undefined.
 * @param err  Error code
 * @returns  true if problem defines the indicated parameter
 */
_BOOL_ tinf_problem_defined(void* problem, const char* key,
                            size_t keylen, enum TINF_DATA_TYPE* datatype,
                            int32_t* rank, size_t dims[TINF_PROBLEM_MAX_RANK],
                            int32_t* err);

/**
 * Retrieve the value(s) associated with a defined named problem parameter.
 *
 * @param problem  Opaque problem pointer
 * @param key  Requested parameter name (length @p keylen)
 * @param keylen  Length of the parameter name
 * @param data  Storage for requested data
 * @param rank  Rank of the parameter
 * @param dims  Number of parameter requested of the given type for each
 *              dimension (length @p rank in @ref tinf_problem_defined).
 *              This should range from 1 to the number defined as given by
 *              @ref tinf_problem_defined
 * @returns  Error code
 */
int32_t tinf_problem_value(void* problem,
                        const char* key, size_t keylen,
                        const void* data, int32_t rank, const size_t dims[]);

/**
 * Set the value(s) associated with a defined named problem variable.
 *
 * @param problem_instance  problem context
 * @param name  Requested variable name (length @p namelen)
 * @param namelen  Length of the variable name
 * @param data  Pointer to data to be set into problem
 * @param rank  Rank of the variable
 * @param dims  Number of variables set of the given type for each
 *              dimension (length @p rank in @ref tinf_problem_defined).
 *              This should range from 1 to the number defined as given by
 *              @ref tinf_problem_defined
 * @param err  Error code
 */
int32_t tinf_problem_set_value(void* problem_instance,
                        const char* name, size_t namelen,
                        const void* data, int32_t rank, const size_t dims[]);

#ifdef __cplusplus
}
#endif

#endif
