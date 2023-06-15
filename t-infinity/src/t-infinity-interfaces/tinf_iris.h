#pragma once
#ifndef TINF_IRIS_H
#define TINF_IRIS_H


/*============================================================================80
 *
 * This file defines the Iris message passing interface
 *
 *============================================================================*/
#include <stdlib.h>
#include "tinf_enum_definitions.h"
#ifdef __cplusplus
#include <complex>
typedef std::complex<double> cmplx_doub_t;
#else
#include <complex.h>
typedef double complex cmplx_doub_t;
#endif

#define tinf_iris_mode_rdonly   MPI_MODE_RDONLY
#define tinf_iris_info_null     MPI_INFO_NULL
#define tinf_iris_byte          MPI_BYTE

#ifdef __cplusplus
#define _BOOL_ bool
#else
#define _BOOL_ _Bool
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create an Iris object based on an MPI communicator.
 *
 * @param comm Opaque Iris pointer
 * @param communicator  MPI Fortran communicator
 * @returns  Error code
 **/
int32_t tinf_iris_create(void** comm, int32_t communicator);

/**
 * Return the Fortran MPI communicator from an opaque Iris pointer.
 *
 * @param comm  Opaque Iris pointer
 * @param err  Error code
 * @returns MPI Fortran communicator
 **/
int32_t tinf_iris_get_mpi_fcomm(const void* comm, int32_t* err);

/**
 * Release MPI Fortran communicator associated to an opaque Iris pointer.
 *
 * @param comm  Opaque Iris pointer
 * @returns Error code
 **/
int32_t tinf_iris_fcomm_free(const void* comm);

/**
 * Retrieve the rank of the calling process in an opaque Iris pointer.
 *
 * @param comm  Opaque Iris pointer
 * @param err  Error code
 * @returns Processor rank (bias-0)
 **/
int32_t tinf_iris_rank(const void* comm, int32_t* err);

/**
 * Retrieve the size of the group associated with an opaque Iris pointer.
 *
 * @param comm  Opaque Iris pointer
 * @param err  Error code
 * @returns Total nuber of processors
 **/
int32_t tinf_iris_number_of_processes(const void* comm, int32_t* err);

/**
 * Stop the processes associated with an opaque Iris pointer based on a
 * nonzero reduction of processor flags.  If any process triggers a stop,
 * then all processes will be stopped.
 *
 * @param comm  Opaque Iris pointer
 * @param flag  Stop flag (nonzero indicates stop requested by process)
 * @param msg  Common message to indicate what has triggered the stop
 * @param len  Length of @p msg character array
 * @returns Error code
 **/
int32_t tinf_iris_stop(const void* comm, int32_t flag, const char* msg,
                       size_t len);

/**
 * Terminate execution environment.
 *
 * @param comm  Opaque Iris pointer
 * @returns Error code
 **/
int32_t tinf_iris_abort(const void* comm);

/**
 * Destroy the Opaque Iris pointer and release its state.
 *
 * @param comm  Opaque Iris pointer
 * @returns Error code
 **/
int32_t tinf_iris_destroy(const void* comm);

/**
 * Create a synchronization barrier for the processes.
 *
 * @param comm  Opaque Iris pointer
 * @returns Error code
 **/
int32_t tinf_iris_barrier(const void* comm);

/**
 * Waits for all given process requests to complete.
 *
 * @param comm  Opaque Iris pointer
 * @param counts  List length
 * @param nreq  Length of @p array_of_requests
 * @param array_of_requests  Array of requests
 * @returns Error code
 **/
int32_t tinf_iris_waitall(const void* comm, int32_t counts, size_t nreq,
                          int32_t array_of_requests);

/**
 * Retrieve the number of characters in the maximum processor name.
 * Used to allocate processor name character array for
 * @ref tinf_iris_get_processor_name.
 *
 * @param comm  Opaque Iris pointer
 * @param err  Error code
 * @returns Length of maximum processor name
 **/
size_t tinf_iris_max_processor_name_length(void* comm, int32_t* err);

/**
 * Retrieve the processor name.
 *
 * @param comm  Opaque Iris pointer
 * @param name  Processor name
 * @param nlen  Number of characters in the processor name
 * @returns Error code
 **/
int32_t tinf_iris_get_processor_name(void* comm, const char* name, size_t nlen);

/**
 * Broadcasts a message from process rank @p root to all other
 * processes of the group.
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param buf  Pointer to storage for @p data_type value(s)
 * @param root  Source rank of data
 * @returns Error code
 **/
int32_t tinf_iris_broadcast(const void* comm, const enum TINF_DATA_TYPE data_type,
                            const int32_t data_rank,
                            const size_t data_dims[TINF_DATA_MAX_RANK],
                            const void* buf, int32_t root);

/**
 * Sum a value(s) via reduction over all processes within a group.
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param buf  Pointer to storage for @p data_type value(s)
 * @param sum  Reduced summation of value(s) across all processes
 * @returns Error code
 **/
int32_t tinf_iris_sum(const void* comm, const enum TINF_DATA_TYPE data_type,
                      const int32_t data_rank, const size_t data_dims[TINF_DATA_MAX_RANK],
                      const void* buf, void* sum);

/**
 * Find the minimum value(s) via reduction over all processes within a group.
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param buf  Pointer to storage for @p data_type value(s)
 * @param min  Reduced minimum value(s) across all processes
 * @returns Error code
 **/
int32_t tinf_iris_min(const void* comm, const enum TINF_DATA_TYPE data_type,
                 const int32_t data_rank,
                 const size_t data_dims[TINF_DATA_MAX_RANK],
                 const void* buf, void* min);

/**
 * Find the maximum value(s) via reduction over all processes within a group.
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param buf  Pointer to storage for @p data_type value(s)
 * @param max  Reduced maximum value(s) across all processes
 * @returns Error code
 **/
int32_t tinf_iris_max(const void* comm, const enum TINF_DATA_TYPE data_type,
                      const int32_t data_rank,
                      const size_t data_dims[TINF_DATA_MAX_RANK],
                      void* buf, void* max);

/**
 * Find the processor rank containing the maximum value(s) found via
 * reduction over all processes within a group.
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param buf  Pointer to storage for @p data_type value(s)
 * @param rank  Rank of the maximum value(s) across all processes
 * @returns Error code
 **/
int32_t tinf_iris_rank_of_max(const void* comm,
                              const enum TINF_DATA_TYPE data_type,
                              const int32_t data_rank,
                              const size_t data_dims[TINF_DATA_MAX_RANK],
                              void* buf, int32_t* rank);

/**
 * Build the data synchronization send/receive pairs that drive the exchange
 * of data between processes.
 *
 * @param comm  Opaque Iris pointer
 * @param global_ids  Array of global identifiers for data points
 * @param global_cnt  Number of global identifiers provided in @p global_ids
 * @param have  Array of local identifiers for data points residing in the
 *              calling process
 * @param have_cnt  Number of residing local identifiers provided in @p have
 * @param need  Array of remote identifiers for data points residing in other
 *              processes
 * @param need_cnt  Number of remote identifiers provided in @p need
 * @param err  Error code
 * @returns Opaque pointer to synchronization pattern
 **/
void* tinf_iris_build_sync_pattern(const void *comm,
                                   const int64_t *global_ids,
                                   const size_t global_cnt,
                                   const int64_t *have, const size_t have_cnt,
                                   const int64_t *need, const size_t need_cnt,
                                   int32_t* err);
/**
 * Destroy the data synchronization send/receive pairs created with
 * @ref tinf_iris_build_sync_pattern.
 *
 * @param comm  Opaque Iris pointer
 * @param sync_pattern  Synchronization pattern to delete
 * @returns Error code
 **/
int32_t tinf_iris_delete_sync_pattern(void *comm, void *sync_pattern);

/**
 * Synchronize data across processes.
 *
 * @param comm  Opaque Iris pointer
 * @param sync_pattern  Synchronization pattern to delete
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param buf  Pointer to storage for @p data_type value(s)
 * @returns Error code
 **/
int32_t tinf_iris_sync(const void* comm, const void* sync_pattern,
                       const enum TINF_DATA_TYPE data_type,
                       const int32_t data_rank,
                       const size_t data_dims[TINF_DATA_MAX_RANK],
                       void* buf);

/**
 * Perform a blocking send.
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param buf  Pointer to storage for @p data_type value(s)
 * @param dest  Process rank of the destination
 * @param tag  Message tag
 * @returns Error code
 **/
int32_t tinf_iris_send(const void* comm, const enum TINF_DATA_TYPE data_type,
                       const int32_t data_rank,
                       const size_t data_dims[TINF_DATA_MAX_RANK],
                       void* buf, const int32_t dest,
                       int32_t tag);

/**
 * Perform a blocking receive.
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param buf  Pointer to storage for @p data_type value(s)
 * @param src  Process rank of the data source
 * @param tag  Message tag
 * @param recv_tag  Message tag of the receive message
 * @returns Error code
 **/
int32_t tinf_iris_recv(const void *comm, const enum TINF_DATA_TYPE data_type,
                       const int32_t data_rank,
                       const size_t data_dims[TINF_DATA_MAX_RANK],
                       void* buf, const int32_t src,
                       int32_t tag, int32_t* recv_tag);

/**
 * Perform a nonblocking send.
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param buf  Pointer to storage for @p data_type value(s)
 * @param dest  Process rank of the destination
 * @param tag  Message tag
 * @param request  Communication request handle
 * @param off  Offset in @p buf of starting address to send
 * @param dev_addr  Device address pointer for buf if using accelerator  
 * @returns Error code
 **/
int32_t tinf_iris_isend(const void* comm, const enum TINF_DATA_TYPE data_type,
                        const int32_t data_rank,
                        const size_t data_dims[TINF_DATA_MAX_RANK],
                        void* buf, const int32_t dest,
                        int32_t tag, void* request,
                        const size_t off[TINF_DATA_MAX_RANK],
                        void* dev_addr); 

/**
 * Perform a nonblocking receive.
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param buf  Pointer to storage for @p data_type value(s)
 * @param src  Process rank of the data source
 * @param tag  Message tag
 * @param request  Communication request handle
 * @param off  Offset in @p buf of starting address to send
 * @param dev_addr  Device address pointer for buf if using accelerator  
 * @returns Error code
 **/
int32_t tinf_iris_irecv(const void* comm, const enum TINF_DATA_TYPE data_type,
                        const int32_t data_rank,
                        const size_t data_dims[TINF_DATA_MAX_RANK],
                        void* buf, const int32_t src,
                        int32_t tag, void* request,
                        const size_t off[TINF_DATA_MAX_RANK],
                        void* dev_addr);

/*
 * Opens a file (collective).
 *
 * @param comm  Opaque Iris pointer
 * @param filename  Name for the file to open
 * @param nlen  Length of @p filename
 * @param mode  File access mode
 * @param info  Opaque info pointer
 * @param file_descriptor  File descriptor
 * @returns Error code
 */
int32_t tinf_iris_file_open(const void* comm, const char* filename,
                            const size_t nlen, const int32_t mode,
                            const void* info, int32_t* file_descriptor);

/*
 * Closes a file (collective).
 *
 * @param comm  Opaque Iris pointer
 * @param file_descriptor  File descriptor
 * @returns Error code
 */
int32_t tinf_iris_file_close(const void* comm, const int32_t file_descriptor);

/* 
 * Changes process’s view of data in file (collective).
 *
 * @param comm  Opaque Iris pointer
 * @param file_descriptor  File descriptor
 * @param disp  Displacement
 * @param etype  Elemental type
 * @param filetype  File type
 * @param datarep  Data representation
 * @param datarep_len  Length of the data representation
 * @param info  Opaque info pointer
 * @returns Error code
 */
int32_t tinf_iris_file_set_view(const void* comm, const int32_t file_descriptor,
                                const int64_t disp, const int32_t etype,
                                const int32_t filetype,
                                char* datarep, size_t datarep_len,
                                void* info);

/*
 *
 */
void* tinf_iris_create_from_ranks(const void* comm, const size_t offset,
                                  const size_t cnt, const int64_t rank_list,
                                  const int32_t* err);

/*
 *
 */
int32_t tinf_iris_file_read_at(void *comm, enum TINF_DATA_TYPE data_type,
                              int32_t file_descriptor, size_t offset,
                              void* buffer, size_t count);

/*
 *
 */
int32_t tinf_iris_file_read_at_all(void *comm, enum TINF_DATA_TYPE data_type,
                                   int32_t file_descriptor, size_t offset,
                                   void* buffer, size_t count);

/**
 * Gathers values from a group of processes.
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param send  Pointer to storage for @p data_type value(s) to send
 * @param recv  Pointer to storage for @p data_type value(s) to receive
 * @param root  Source rank of receiving process
 * @returns Error code
 **/
int32_t tinf_iris_gather(const void *comm, const enum TINF_DATA_TYPE data_type,
                         const int32_t data_rank,
                         const size_t data_dims[TINF_DATA_MAX_RANK],
                         void *send, void *recv, int32_t rank);

/**
 * All processes send data to all processes
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param send  Pointer to storage for @p data_type value(s) to send
 * @param recv  Pointer to storage for @p data_type value(s) to receive
 * @returns Error code
 **/
int32_t tinf_iris_alltoall(const void *comm,
                           const enum TINF_DATA_TYPE data_type,
                           const int32_t data_rank,
                           const size_t data_dims[TINF_DATA_MAX_RANK],
                           void *send, void *recv);

/**
 * Sends data from all to all processes; each process may send a different
 * amount of data and provide displacements for the input and output data.
 *
 * @param comm  Opaque Iris pointer
 * @param data_type  Data type of value(s)
 * @param data_rank   Rank of the data value(s) (0 - scalar, 1 - array, etc.)
 * @param data_dims   Dimension for each @p data_rank
 * @param send  Pointer to storage for @p data_type value(s) to send
 * @param nsend  Array of length @ref tinf_iris_number_of_processes
 *              specifying the number of elements to send to each processor
 * @param recv  Pointer to storage for @p data_type value(s) to receive
 * @param nrecv  Array of length @ref tinf_iris_number_of_processes
 *               specifying the number of elements to send to each processor
 * @returns Error code
 **/
int32_t tinf_iris_alltoallv(const void *comm,
                            const enum TINF_DATA_TYPE data_type,
                            const int32_t data_rank,
                            const size_t data_dims[TINF_DATA_MAX_RANK],
                            void *send, const int32_t* nsend,
                            void *recv, const int32_t* nrecv);

#ifdef __cplusplus
} /* Close extern "C" declaration. */
#endif

#endif
