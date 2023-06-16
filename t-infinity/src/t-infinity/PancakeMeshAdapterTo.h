#include <memory>
#include <iostream>
#include <array>
#include <t-infinity/MeshInterface.h>
#include <t-infinity-interfaces/tinf_enum_definitions.h>
#include "Mangle.h"

extern "C" {

int64_t mangle(tinf_mesh_node_count)(void* const mesh, int32_t* error);
int64_t mangle(tinf_mesh_resident_node_count)(void* const mesh, int32_t* error);
int64_t mangle(tinf_mesh_element_count)(void* const mesh, int32_t* error);
int64_t mangle(tinf_mesh_element_type_count)(void* const mesh,
                                             const enum TINF_ELEMENT_TYPE type,
                                             int32_t* error);
int32_t mangle(tinf_mesh_nodes_coordinates)(void* const mesh,
                                            const enum TINF_DATA_TYPE data_type,
                                            const int64_t start,
                                            const int64_t cnt,
                                            void* x,
                                            void* y,
                                            void* z);
TINF_ELEMENT_TYPE mangle(tinf_mesh_element_type)(void* const mesh,
                                                 const int64_t element_id,
                                                 int32_t* error);
int32_t mangle(tinf_mesh_element_nodes)(void* const mesh,
                                        const int64_t element_id,
                                        int64_t* element_nodes);
int64_t mangle(tinf_mesh_global_node_id)(void* const mesh, const int64_t local_id, int32_t* error);
int64_t mangle(tinf_mesh_global_element_id)(void* const mesh,
                                            const int64_t local_id,
                                            int32_t* error);
int64_t mangle(tinf_mesh_element_tag)(void* const mesh, const int64_t element_id, int32_t* error);
int64_t mangle(tinf_mesh_element_owner)(void* const mesh, const int64_t element_id, int32_t* error);
int64_t mangle(tinf_mesh_node_owner)(void* const mesh, const int64_t node_id, int32_t* error);
int64_t mangle(tinf_mesh_partition_id)(void* const mesh, int32_t* error);
}

int64_t mangle(tinf_mesh_node_count)(void* const mesh, int32_t* error) {
    int64_t count = -1;
    *error = TINF_SUCCESS;
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        count = m->shared_ptr->nodeCount();
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        *error = TINF_FAILURE;
    }
    return count;
}

int64_t mangle(tinf_mesh_resident_node_count)(void* const mesh, int32_t* error) {
    int64_t count = -1;
    *error = TINF_SUCCESS;
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        count = 0;
        for (int i = 0; i < m->shared_ptr->nodeCount(); i++)
            if (m->shared_ptr->partitionId() == m->shared_ptr->nodeOwner(i)) count += 1;
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        *error = TINF_FAILURE;
    }
    return count;
}

int64_t mangle(tinf_mesh_element_count)(void* const mesh, int32_t* error) {
    int64_t count = -1;
    *error = TINF_SUCCESS;
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        count = m->shared_ptr->cellCount();
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        *error = TINF_FAILURE;
    }
    return count;
}

int64_t mangle(tinf_mesh_element_type_count)(void* const mesh,
                                             const enum TINF_ELEMENT_TYPE type,
                                             int32_t* error) {
    int64_t count = -1;
    *error = TINF_SUCCESS;
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        count = 0;
        for (int i = 0; i < m->shared_ptr->cellCount(); i++)
            if (m->shared_ptr->cellType(i) == inf::MeshInterface::CellType(type)) count += 1;
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        *error = TINF_FAILURE;
    }
    return count;
}

void setAsDouble(double* x, double* y, double* z, int i, double* p) {
    x[i] = p[0];
    y[i] = p[1];
    z[i] = p[2];
}

void setAsComplex(double* x, double* y, double* z, int i, double* p) {
    x[2 * i] = p[0];
    y[2 * i] = p[1];
    z[2 * i] = p[2];
    x[2 * i + 1] = 0;
    y[2 * i + 1] = 0;
    z[2 * i + 1] = 0;
}

int32_t mangle(tinf_mesh_nodes_coordinates)(void* const mesh,
                                            const enum TINF_DATA_TYPE type,
                                            const int64_t start,
                                            const int64_t count,
                                            void* x,
                                            void* y,
                                            void* z) {
    auto* actual_x = static_cast<double*>(x);
    auto* actual_y = static_cast<double*>(y);
    auto* actual_z = static_cast<double*>(z);
    auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
    std::array<double, 3> p{};

    if (type == TINF_DOUBLE or type == TINF_CMPLX_DOUBLE) {
        try {
            for (int i = 0; i < count; i++) {
                m->shared_ptr->nodeCoordinate(start + i, p.data());
                if (type == TINF_DOUBLE)
                    setAsDouble(actual_x, actual_y, actual_z, i, p.data());
                else
                    setAsComplex(actual_x, actual_y, actual_z, i, p.data());
            }
        } catch (std::exception& e) {
            std::cout << "Caught Error:" << e.what() << std::endl;
            return TINF_FAILURE;
        }
    } else {
        printf("T-Infinity preprocessor only provides type TINF_DOUBLE or TINF_COMPLEX\n");
        return TINF_FAILURE;
    }
    return TINF_SUCCESS;
}

TINF_ELEMENT_TYPE mangle(tinf_mesh_element_type)(void* const mesh,
                                                 const int64_t element_id,
                                                 int32_t* error) {
    *error = TINF_SUCCESS;
    TINF_ELEMENT_TYPE type = TINF_NODE;
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        type = TINF_ELEMENT_TYPE(m->shared_ptr->cellType(element_id));
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        *error = TINF_FAILURE;
    }
    return type;
}

int32_t mangle(tinf_mesh_element_nodes)(void* const mesh,
                                        const int64_t element_id,
                                        int64_t* element_nodes) {
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        auto type = m->shared_ptr->cellType(element_id);
        auto length = inf::MeshInterface::cellTypeLength(type);
        std::vector<int32_t> nodes(length);
        m->shared_ptr->cell(element_id, nodes.data());
        for (int32_t i = 0; i < length; i++) element_nodes[i] = nodes[i];
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        return TINF_FAILURE;
    }
    return TINF_SUCCESS;
}

int64_t mangle(tinf_mesh_global_node_id)(void* const mesh, const int64_t local_id, int32_t* error) {
    *error = TINF_SUCCESS;
    int64_t global_id = -1;
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        global_id = m->shared_ptr->globalNodeId(local_id);
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        *error = TINF_FAILURE;
    }
    return global_id;
}

int64_t mangle(tinf_mesh_global_element_id)(void* const mesh,
                                            const int64_t local_id,
                                            int32_t* error) {
    *error = TINF_SUCCESS;
    int64_t global_id = -1;
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        global_id = m->shared_ptr->globalCellId(local_id);
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        *error = TINF_FAILURE;
    }
    return global_id;
}

int64_t mangle(tinf_mesh_element_tag)(void* const mesh, const int64_t element_id, int32_t* error) {
    *error = TINF_SUCCESS;
    int64_t tag = -1;
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        tag = m->shared_ptr->cellTag(element_id);
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        *error = TINF_FAILURE;
    }
    return tag;
}

int64_t mangle(tinf_mesh_element_owner)(void* const mesh,
                                        const int64_t element_id,
                                        int32_t* error) {
    *error = TINF_SUCCESS;
    int64_t owner = -1;
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        owner = m->shared_ptr->cellOwner(element_id);
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        *error = TINF_FAILURE;
    }
    return owner;
}

int64_t mangle(tinf_mesh_node_owner)(void* const mesh, const int64_t node_id, int32_t* error) {
    *error = TINF_SUCCESS;
    int64_t owner = -1;
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        owner = m->shared_ptr->nodeOwner(node_id);
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        return TINF_FAILURE;
        *error = TINF_FAILURE;
    }
    return owner;
}

int64_t mangle(tinf_mesh_partition_id)(void* const mesh, int32_t* error) {
    *error = TINF_SUCCESS;
    int64_t partition = -1;
    try {
        auto m = static_cast<SharedPointerWrapper<inf::MeshInterface>*>(mesh);
        partition = m->shared_ptr->partitionId();
    } catch (std::exception& e) {
        std::cout << "Caught Error:" << e.what() << std::endl;
        *error = TINF_FAILURE;
    }
    return partition;
}
