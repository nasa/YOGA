#include "YogaMesh.h"
#include <stdlib.h>
#include <algorithm>
#include <functional>
#include <set>
#include "BoundaryConditions.h"

namespace YOGA {
void YogaMesh::setNodeCount(int n) {
    nodes.resize(n);
    global_node_ids.resize(n);
    node_owning_rank.resize(n);
    component_id_for_node.resize(n);
    is_ghost_node.resize(n);
}

void YogaMesh::setCellCount(int n) { ncells = n; }

void YogaMesh::setFaceCount(int n) { nfaces = n; }

void YogaMesh::setXyzForNodes(std::function<void(int, double*)> get_node) {
    for (int i = 0; i < nodeCount(); i++) get_node(i, nodes[i].data());
}

void YogaMesh::setGlobalNodeIds(std::function<long(int)> get_global_node_id) {
    for (int i = 0; i < nodeCount(); i++) global_node_ids[i] = get_global_node_id(i);
}

void YogaMesh::setOwningRankForNodes(std::function<int(int)> get_owning_rank_for_node) {
    for (int i = 0; i < nodeCount(); i++) node_owning_rank[i] = get_owning_rank_for_node(i);
}

void YogaMesh::setBoundaryConditions(std::function<int(int)> get_boundary_condition,
                                    std::function<int(int)> number_of_nodes_in_boundary_face) {
    triangle_bcs.resize(triangles.size());
    quad_bcs.resize(quads.size());
    int triangle_id = 0;
    int quad_id = 0;
    for(int i=0;i<nfaces;i++){
        auto bc = (YOGA::BoundaryConditions)get_boundary_condition(i);
        int n = number_of_nodes_in_boundary_face(i);
        if(3 == n){
            triangle_bcs[triangle_id++] = bc;
        }
        else{
            quad_bcs[quad_id++] = bc;
        }
    }
}

void YogaMesh::setFaces(std::function<int(int)> number_of_nodes_in_boundary_face,
                        std::function<void(int, int*)> get_nodes_in_boundary_face) {
    for (int i = 0; i < nfaces; i++) {
        int n = number_of_nodes_in_boundary_face(i);
        addFace(i, n, get_nodes_in_boundary_face);
    }
}

void YogaMesh::setCells(std::function<int(int)> number_of_nodes_in_cell,
                        std::function<void(int, int*)> get_nodes_in_cell) {
    for (int i = 0; i < ncells; i++) {
        int n = number_of_nodes_in_cell(i);
        addCell(i, n, get_nodes_in_cell);
    }
}

void YogaMesh::setComponentIdsForNodes(std::function<int(int)> get_comp_id_f) {
    for (int i = 0; i < nodeCount(); i++){
        component_id_for_node[i] = get_comp_id_f(i);
    }
}

void YogaMesh::addFace(int face_id, int face_size, std::function<void(int, int*)> get_nodes_in_boundary_face) {
    if (3 == face_size) {
        int ntri = triangles.size();
        triangles.resize(ntri + 1);
        get_nodes_in_boundary_face(face_id, triangles.back().data());
        face_types.push_back({TRIANGLE, ntri});
    } else {
        int nquad = quads.size();
        quads.resize(nquad + 1);
        get_nodes_in_boundary_face(face_id, quads.back().data());
        face_types.push_back({QUAD, nquad});
    }
}

void YogaMesh::addCell(int cell_id, int cell_size, std::function<void(int, int*)> get_nodes_in_cell) {
    if (4 == cell_size) {
        int ntets = tets.size();
        tets.resize(ntets + 1);
        get_nodes_in_cell(cell_id, tets.back().data());
        tet_cell_ids.push_back(cell_id);
        cell_types.push_back({TET, ntets});
    } else if (5 == cell_size) {
        int npyramids = pyramids.size();
        pyramids.resize(npyramids + 1);
        get_nodes_in_cell(cell_id, pyramids.back().data());
        pyramid_cell_ids.push_back(cell_id);
        cell_types.push_back({PYRAMID, npyramids});
    } else if (6 == cell_size) {
        int nprisms = prisms.size();
        prisms.resize(nprisms + 1);
        get_nodes_in_cell(cell_id, prisms.back().data());
        prism_cell_ids.push_back(cell_id);
        cell_types.push_back({PRISM, nprisms});
    } else if (8 == cell_size) {
        int nhexs = hexs.size();
        hexs.resize(nhexs + 1);
        get_nodes_in_cell(cell_id, hexs.back().data());
        hex_cell_ids.push_back(cell_id);
        cell_types.push_back({HEX, nhexs});
    } else {
        throw std::logic_error(std::string(__FUNCTION__) + ": bad cell size: " + std::to_string(cell_size));
    }
}

int YogaMesh::nodeCount() const { return nodes.size(); }

int YogaMesh::getAssociatedComponentId(int node_id) const { return component_id_for_node[node_id]; }


void YogaMesh::nodeCoordinate(int node_id,double* p) const {
    for(int i=0;i<3;i++)
        p[i] = nodes[node_id][i];
}

int YogaMesh::numberOfBoundaryFaces() const { return nfaces; }

auto YogaMesh::getFaceType(int id) const { return face_types[id].first; }

int YogaMesh::getFaceIdInType(int id) const { return face_types[id].second; }

YOGA::BoundaryConditions YogaMesh::getBoundaryCondition(int face_id) const {
    int face_type = getFaceType(face_id);
    int id_in_type = getFaceIdInType(face_id);
    if (TRIANGLE == face_type)
        return triangle_bcs[id_in_type];
    else if (QUAD == face_type)
        return quad_bcs[id_in_type];
    throw std::logic_error("Bad face type");
}

std::vector<int> YogaMesh::getNodesInBoundaryFace(int face_id) const {
    auto face_type = getFaceType(face_id);
    int id_in_type = getFaceIdInType(face_id);
    std::vector<int> face(numberOfNodesInBoundaryFace(face_id));
    if (TRIANGLE == face_type)
        std::copy(triangles[id_in_type].begin(), triangles[id_in_type].end(), face.data());
    else if (QUAD == face_type)
        std::copy(quads.at(id_in_type).begin(), quads.at(id_in_type).end(), face.data());
    else
        throw std::logic_error("Bad face type");
    return face;
}

int YogaMesh::numberOfNodesInBoundaryFace(int id) const {
    if (TRIANGLE == getFaceType(id)) return 3;
    return 4;
}

int YogaMesh::numberOfCells() const { return ncells; }

int YogaMesh::cellCount(CellType cell_type) const {
    switch (cell_type) {
        case TET:
            return tets.size();
        case PYRAMID:
            return pyramids.size();
        case PRISM:
            return prisms.size();
        case HEX:
            return hexs.size();
        default:
            return 0;
    }
}

auto YogaMesh::getCellType(int cell_id) const { return cell_types[cell_id].first; }

int YogaMesh::getCellIdInType(int cell_id) const { return cell_types[cell_id].second; }

void YogaMesh::getNodesInCell(int cell_id, int* cell) const {
    if(cell_id < 0 or cell_id > numberOfCells())
        throw std::logic_error("Cell id out of range: "+std::to_string(cell_id));
    auto cell_type = getCellType(cell_id);
    int id_in_type = getCellIdInType(cell_id);
    if (TET == cell_type)
        std::copy(tets[id_in_type].begin(), tets[id_in_type].end(), cell);
    else if (PYRAMID == cell_type)
        std::copy(pyramids[id_in_type].begin(), pyramids[id_in_type].end(), cell);
    else if (PRISM == cell_type)
        std::copy(prisms[id_in_type].begin(), prisms[id_in_type].end(), cell);
    else if (HEX == cell_type)
        std::copy(hexs[id_in_type].begin(), hexs[id_in_type].end(), cell);
}

const int* YogaMesh::cell_ptr(int cell_id) const {
    auto cell_type = getCellType(cell_id);
    int id_in_type = getCellIdInType(cell_id);
    if (TET == cell_type)
        return tets[id_in_type].data();
    else if (PYRAMID == cell_type)
        return pyramids[id_in_type].data();
    else if (PRISM == cell_type)
        return prisms[id_in_type].data();
    else if (HEX == cell_type)
        return hexs[id_in_type].data();
    return nullptr;
}
int YogaMesh::numberOfNodesInCell(int id) const {
    auto cell_type = getCellType(id);
    if (TET == cell_type) return 4;
    if (PYRAMID == cell_type) return 5;
    if (PRISM == cell_type) return 6;
    if (HEX == cell_type) return 8;
    return 0;
}

long YogaMesh::globalNodeId(int node_id) const { return global_node_ids[node_id]; }

int YogaMesh::nodeOwner(int node_id) const { return node_owning_rank[node_id]; }
int YogaMesh::getCellIdFromOriginalMesh(int cell_id) const {
    auto cell_type = getCellType(cell_id);
    int id_in_type = getCellIdInType(cell_id);
    if(TET == cell_type)
        return tet_cell_ids.at(id_in_type);
    else if(PYRAMID == cell_type)
        return pyramid_cell_ids.at(id_in_type);
    else if(PRISM == cell_type)
        return prism_cell_ids.at(id_in_type);
    else if(HEX == cell_type)
        return hex_cell_ids.at(id_in_type);
    else
        throw std::logic_error(__FUNCTION__);
}

void YogaMesh::fixInvalidFun3DComponentIds(MessagePasser mp) {
    std::set<int> unique_ids(component_id_for_node.begin(),component_id_for_node.end());
    unique_ids = mp.ParallelUnion(unique_ids);
    bool component_ids_start_at_zero = unique_ids.count(0) == 1;
    if(not component_ids_start_at_zero)
        for(int& id:component_id_for_node)
            id--;
}
void YogaMesh::setComplexGetter(std::function<void(int, double*)> get_node) {
    complex_getter = get_node;
}

}
