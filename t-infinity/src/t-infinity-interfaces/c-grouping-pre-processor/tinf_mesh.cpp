#include <memory>
#include <t-infinity/MeshInterface.h>
#include <tinf_mesh.h>
#include <parfait/Checkpoint.h>
#include <array>

std::shared_ptr<inf::MeshInterface> pointerCast(void* const handle) {
    if (handle == NULL) {
        throw std::logic_error("Could not convert mesh handle to inf::MeshInterface.  Handle is null");
    }

    std::shared_ptr<inf::MeshInterface>* casted_ptr =
        static_cast<std::shared_ptr<inf::MeshInterface>*>(handle);

    if (casted_ptr == nullptr) {
        throw std::logic_error("Could not convert mesh handle to inf::MeshInterface.  Cast is null");
    }
    return *casted_ptr;
}

int32_t tinf_mesh_node_count(void* const handle, int64_t* count) {
    auto mesh = pointerCast(handle);
    auto n = mesh->nodeCount();
    *count = int64_t(n);
    return TINF_SUCCESS;
}

int32_t tinf_mesh_resident_node_count(void* const handle, int64_t* count) {
    auto mesh = pointerCast(handle);
    int owned_count = 0;
    for (int n = 0; n < mesh->nodeCount(); n++) {
        if (mesh->ownedNode(n)) owned_count++;
    }

    *count = int64_t(owned_count);
    return TINF_SUCCESS;
}

int32_t tinf_mesh_element_count(void* const handle, int64_t* count) {
    auto mesh = pointerCast(handle);
    auto n = mesh->cellCount();
    *count = int64_t(n);
    return TINF_SUCCESS;
}

TINF_ELEMENT_TYPE inf_type_to_c_type(inf::MeshInterface::CellType type){
    int int_type(type);
    auto c_type = TINF_ELEMENT_TYPE(int_type);
    return c_type;
}

inf::MeshInterface::CellType c_type_to_inf_type(TINF_ELEMENT_TYPE type){
    int int_type = int(type);
    auto inf_type = inf::MeshInterface::CellType(int_type);
    return inf_type;
}

int32_t tinf_mesh_element_type_count(void* const handle, const enum TINF_ELEMENT_TYPE type, int64_t* count) {
    auto mesh = pointerCast(handle);
    int type_count = 0;
    auto inf_type = c_type_to_inf_type(type);
    for (int c = 0; c < mesh->cellCount(); c++) {
        auto t = mesh->cellType(c);
        if (t == inf_type) type_count++;
    }

    *count = int64_t(type_count);
    return TINF_SUCCESS;
}

int32_t tinf_mesh_nodes_coordinates(void* const handle,
                                    const enum TINF_DATA_TYPE data_type,
                                    const int64_t start,
                                    const int64_t cnt,
                                    void* x_in,
                                    void* y_in,
                                    void* z_in) {
    auto mesh = pointerCast(handle);
    if (data_type != TINF_DOUBLE) {
        printf("T-infinity c++ pre-processor only handles double valued meshes.");
        return TINF_FAILURE;
    }

    auto* x = (double*)x_in;
    auto* y = (double*)y_in;
    auto* z = (double*)z_in;

    size_t offset = 0;
    for (int n = int(start); n < int(start + cnt); n++) {
        std::array<double, 3> p;
        mesh->nodeCoordinate(n, p.data());
        x[offset] = p[0];
        y[offset] = p[1];
        z[offset] = p[2];
        offset++;
    }

    return TINF_SUCCESS;
}

int32_t tinf_mesh_nodes_distance(
    void* const mesh, const enum TINF_DATA_TYPE data_type, const int64_t start, const int64_t cnt, void* slen){
    printf("ERROR: T-infinity c++ pre-processor does not calculate distance.\n");
    return TINF_FAILURE;
}

int32_t tinf_mesh_element_type(void* const handle, const int64_t element_id, enum TINF_ELEMENT_TYPE* type){
    auto mesh = pointerCast(handle);
    auto t = mesh->cellType(int(element_id));
    *type = inf_type_to_c_type(t);
    return TINF_SUCCESS;
}

int32_t tinf_mesh_element_nodes(void* const handle, const int64_t element_id, int64_t* element_nodes){
    auto mesh = pointerCast(handle);
    std::vector<int> cell_nodes(mesh->cellLength(int(element_id)));
    mesh->cell(element_id, cell_nodes.data());
    for(size_t i = 0; i < cell_nodes.size(); i++){
        element_nodes[i] = int64_t(cell_nodes[i]);
    }
    return TINF_SUCCESS;
}

int32_t tinf_mesh_global_node_id(void* const handle, const int64_t local_id, int64_t* global_id){
    auto mesh = pointerCast(handle);
    *global_id = int64_t(mesh->globalNodeId(int(local_id)));
    return TINF_SUCCESS;
}

int32_t tinf_mesh_global_element_id(void* const handle, const int64_t local_id, int64_t* global_id){
    auto mesh = pointerCast(handle);
    *global_id = int64_t(mesh->globalCellId(int(local_id)));
    return TINF_SUCCESS;
}

int32_t tinf_mesh_element_tag(void* const handle, const int64_t element_id, int64_t* tag){
    auto mesh = pointerCast(handle);
    *tag = int64_t(mesh->cellTag(int(element_id)));
    return TINF_SUCCESS;
}

int32_t tinf_mesh_element_owner(void* const handle, const int64_t element_id, int64_t* partition){
    auto mesh = pointerCast(handle);
    *partition = int64_t(mesh->cellOwner(int(element_id)));
    return TINF_SUCCESS;
}

int32_t tinf_mesh_node_owner(void* const handle, const int64_t node_id, int64_t* partition){
    auto mesh = pointerCast(handle);
    *partition = int64_t(mesh->nodeOwner(int(node_id)));
    return TINF_SUCCESS;
}

int32_t tinf_mesh_partition_id(void* const handle, int64_t* partition){
    auto mesh = pointerCast(handle);
    *partition = int64_t(mesh->partitionId());
    return TINF_SUCCESS;
}
