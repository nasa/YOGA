#pragma once
#ifndef TINF_MESH_H
#define TINF_MESH_H

#include <stdlib.h>

#include "tinf_enum_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef int64_t (*tinf_mesh_node_count_f)(void* mesh, int32_t* error);
    typedef int64_t (*tinf_mesh_resident_node_count_f)(void* mesh, int32_t* error);
    typedef int64_t (*tinf_mesh_element_count_f)(void* mesh, int32_t* error);
    typedef int64_t (*tinf_mesh_element_type_count_f)(void* mesh, enum TINF_ELEMENT_TYPE type, int32_t* error);
    typedef int32_t (*tinf_mesh_nodes_coordinates_f)(void* mesh, enum TINF_DATA_TYPE type, int64_t start, int64_t cnt, void* x, void* y, void* z);
    typedef enum TINF_ELEMENT_TYPE (*tinf_mesh_element_type_f)(void* mesh, int64_t element_id, int32_t* error);
    typedef int32_t (*tinf_mesh_element_nodes_f)(void* mesh, int64_t element_id, int64_t* element_nodes);
    typedef int64_t (*tinf_mesh_global_node_id_f)(void* mesh, int64_t local_id, int32_t* error);
    typedef int64_t (*tinf_mesh_global_element_id_f)(void* mesh, int64_t local_id, int32_t* error);
    typedef int64_t (*tinf_mesh_element_tag_f)(void* mesh, int64_t element_id, int32_t* error);
    typedef int64_t (*tinf_mesh_element_owner_f)(void* mesh, int64_t element_id, int32_t* error);
    typedef int64_t (*tinf_mesh_node_owner_f)(void* mesh, int64_t node_id, int32_t* error);
    typedef int64_t (*tinf_mesh_partition_id_f)(void* mesh, int32_t* error);


    /**
     * @subsection mesh_interface Mesh Interface
     **/

    /**
     * Retrieve the total number of nodes in the mesh partition (resident and
     * neighboring/ghost nodes).
     *
     * @param mesh  Opaque mesh pointer
     * @param error  Error code
     * @returns  Total number of nodes in the partition
     **/
    int64_t tinf_mesh_node_count(void* const mesh, int32_t* error);


    /**
     * Retrieve the number of nodes residing on (owned by) this partition.
     *
     * @param mesh  Opaque mesh pointer
     * @param error  Error code
     * @returns  Number of nodes owned by this partition
     **/
    int64_t tinf_mesh_resident_node_count(void* const mesh, int32_t* error);


    /**
     * Retrieve the number of elements in the mesh.
     *
     * @param mesh  Opaque mesh pointer
     * @param error  Error code
     * @returns  Number of elements
     **/
    int64_t tinf_mesh_element_count(void* const mesh, int32_t* error);


    /**
     * Retrieve the number of elements of a given element type in the mesh.
     *
     * @param mesh  Opaque mesh pointer
     * @param type  Type of element (@ref TINF_ELEMENT_TYPE) to count
     * @param error  Error code
     * @returns  Number of elements of the given @p type
     **/
    int64_t tinf_mesh_element_type_count(void* const mesh, const enum TINF_ELEMENT_TYPE type, int32_t* error);


    /**
     * Retrieve the physical coordinates of one or more mesh nodes.
     *
     * @param mesh  Opaque mesh pointer
     * @param data_type  identify the data type of the coordinates
     * @param start  Starting local node identifier (bias 0)
     * @param cnt  Number of local nodes to retrieve
     * @param x  X coordinate of the target node (length must be at least
     *           @p cnt)
     * @param y  Y coordinate of the target node (length must be at least
     *           @p cnt)
     * @param z  Z coordinate of the target node (length must be at least
     *           @p cnt)
     * @returns  Error code
     **/
    int32_t tinf_mesh_nodes_coordinates(void* const mesh,
                                        const enum TINF_DATA_TYPE data_type,
                                        const int64_t start, const int64_t cnt,
                                        void* x, void* y, void* z);


    /**
     * Retrieve the element type for a local element in the mesh.
     *
     * @param mesh  Opaque mesh pointer
     * @param element_id  Target local element identifier (bias 0)
     * @param error  Error code
     * @returns  Element type from enum @ref TINF_ELEMENT_TYPE
     **/
    enum TINF_ELEMENT_TYPE tinf_mesh_element_type(void* const mesh,
                                                  const int64_t element_id,
                                                  int32_t* error);

    /**
     * Retrieve the cell-to-node map for an local element in the mesh.
     * Cell windings follow CGNS ordering.
     *
     * @param mesh  Opaque mesh pointer
     * @param element_id  Target local element identifier (bias 0)
     * @param element_nodes  Cell-to-node map for the element whose dimension
     *                       should be large enough to hold the number of nodes
     *                       for the target @ref tinf_mesh_element_type (bias 0)
     * @returns  Error code
     **/
    int32_t tinf_mesh_element_nodes(void* const mesh, const int64_t element_id,
                                    int64_t* element_nodes);

    /**
     * Retrieve the global node identifier for a local node in the mesh.
     *
     * @param mesh  Opaque mesh pointer
     * @param local_id  Target local node identifier (bias 0)
     * @param error  Error code
     * @returns  Global node identifier (bias 0)
     **/
    int64_t tinf_mesh_global_node_id(void* const mesh, const int64_t local_id,
                                     int32_t* error);

    /**
     * Retrieve the global element identifier for a local element in the mesh.
     *
     * @param mesh  Opaque mesh pointer
     * @param local_id  Target local element identifier (bias 0)
     * @param error  Error code
     * @returns  Global element identifier (bias 0)
     **/
    int64_t tinf_mesh_global_element_id(void* const mesh, const int64_t local_id,
                                        int32_t* error);

    /**
     * Retrieve the tag associated with an element in the mesh.
     *
     * @param mesh  Opaque mesh pointer
     * @param element_id  Target element identifier (bias 0)
     * @param error  Error code
     * @returns  Associated tag
     **/
    int64_t tinf_mesh_element_tag(void* const mesh, const int64_t element_id,
                                  int32_t* error);

    /* TODO: clarify names to better convey function */

    /**
     * Retrieve the partition identifier for the mesh.
     *
     * @param mesh  Opaque mesh pointer
     * @param element_id  Target element identifier (bias 0)
     * @param error  Error code
     * @returns  Partition identifier for the element (bias 0)
     **/
    int64_t tinf_mesh_element_owner(void* const mesh, const int64_t element_id,
                                    int32_t* error);

    /**
     * Retrieve the partition associated with a given node in the mesh.
     *
     * @param mesh  Opaque mesh pointer
     * @param node_id  Target node identifier (bias 0)
     * @param error  Error code
     * @returns  Partition identifier for the node (bias 0)
     **/
    int64_t tinf_mesh_node_owner(void* const mesh, const int64_t node_id,
                                 int32_t* error);

    /**
     * Retrieve the partition identifier for the mesh.
     *
     * @param mesh  Opaque mesh pointer
     * @param error  Error code
     * @returns  Partition identifier for the opaque mesh pointer (bias 0)
     **/
    int64_t tinf_mesh_partition_id(void* const mesh, int32_t* error);


    /* cell windings follow CGNS ordering.  All indices (for nodes, cells,
       partitions) follow C-indexing.
       all methods return 0 for success, non-zero for failure.
       currently we do not have methods for global cell counts or global node
       counts.
       A user could get these, but it would require communication.  Should
       they be added? */

#ifdef __cplusplus
}
#endif

#endif
