#pragma once
#include <parfait/Throw.h>
#include <t-infinity/MeshInterface.h>
#include "tinf_mesh.h"
#include "tinf_enum_definitions.h"

namespace inf {

class PancakeMeshAdapterFrom : public MeshInterface {
  public:
    PancakeMeshAdapterFrom(void* mesh_context) : m_context(mesh_context) {}

    virtual int nodeCount() const {
        int32_t error;
        int64_t count = tinf_mesh_node_count(m_context, &error);
        if (error != TINF_SUCCESS) PARFAIT_THROW("tinf_mesh died.");
        return (int)count;
    }

    virtual int cellCount() const {
        int32_t error;
        int64_t count = tinf_mesh_element_count(m_context, &error);
        if (error != TINF_SUCCESS) PARFAIT_THROW("tinf_mesh died.");
        return (int)count;
    }

    virtual int cellCount(CellType cell_type) const {
        int64_t count = 0;
        for (int64_t i = 0; i < cellCount(); ++i) {
            if (cellType((int)i) == cell_type) ++count;
        }
        return (int)count;
    }

    virtual int partitionId() const {
        int32_t error;
        int64_t part = tinf_mesh_partition_id(m_context, &error);
        if (error != TINF_SUCCESS) PARFAIT_THROW("tinf_mesh died.");
        return (int)part;
    }

    virtual void nodeCoordinate(int node_id, double* coord) const {
        int32_t error = tinf_mesh_nodes_coordinates(
            m_context, TINF_DOUBLE, (int64_t)node_id, 1, &coord[0], &coord[1], &coord[2]);
        if (error != TINF_SUCCESS) PARFAIT_THROW("tinf_mesh died.");
    }

    virtual CellType cellType(int cell_id) const {
        int32_t error;
        TINF_ELEMENT_TYPE cell_type = tinf_mesh_element_type(m_context, (int64_t)cell_id, &error);
        if (error != TINF_SUCCESS) PARFAIT_THROW("tinf_mesh died.");
        return (CellType)cell_type;
    }

    virtual void cell(int cell_id, int* cell_out) const {
        int cell_length = cellLength(cell_id);
        std::vector<int64_t> cell_nodes(cell_length);
        int32_t error = tinf_mesh_element_nodes(m_context, (int64_t)cell_id, cell_nodes.data());
        if (error != TINF_SUCCESS) PARFAIT_THROW("tinf_mesh died.");
        for (int i = 0; i < cell_length; ++i) cell_out[i] = (int)cell_nodes[i];
    }

    virtual long globalNodeId(int node_id) const {
        int32_t error;
        int64_t global_id = tinf_mesh_global_node_id(m_context, (int64_t)node_id, &error);
        if (error != TINF_SUCCESS) PARFAIT_THROW("tinf_mesh died.");
        return (long)global_id;
    }

    virtual int cellTag(int cell_id) const {
        int32_t error;
        int64_t tag = tinf_mesh_element_tag(m_context, (int64_t)cell_id, &error);
        if (error != TINF_SUCCESS) PARFAIT_THROW("tinf_mesh died.");
        return (int)tag;
    }

    virtual int nodeOwner(int node_id) const {
        int32_t error;
        int64_t owner = tinf_mesh_node_owner(m_context, (int64_t)node_id, &error);
        if (error != TINF_SUCCESS) PARFAIT_THROW("tinf_mesh died.");
        return (int)owner;
    }

    virtual long globalCellId(int cell_id) const {
        int32_t error;
        int64_t global_id = tinf_mesh_global_element_id(m_context, (int64_t)cell_id, &error);
        if (error != TINF_SUCCESS) PARFAIT_THROW("tinf_mesh died.");
        return (long)global_id;
    }

    virtual int cellOwner(int cell_id) const {
        int32_t error;
        int64_t owner = tinf_mesh_element_owner(m_context, (int64_t)cell_id, &error);
        if (error != TINF_SUCCESS) PARFAIT_THROW("tinf_mesh died.");
        return (int)owner;
    }

    inline void* context(void) const { return m_context; }

  private:
    void* m_context;
};
}
