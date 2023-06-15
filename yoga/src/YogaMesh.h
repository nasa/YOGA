#pragma once

#include <parfait/Point.h>
#include <MessagePasser/MessagePasser.h>
#include <functional>
#include "BoundaryConditions.h"

namespace YOGA {

struct NodeStruct {
    NodeStruct() = default;

    NodeStruct(Parfait::Point<double> p, int id) : p(p), component_id(id) {}

    Parfait::Point<double> p;
    int component_id;
};

class YogaMesh {
  public:
    enum CellType { TET, PYRAMID, PRISM, HEX };
    enum FaceType { TRIANGLE, QUAD };

    YogaMesh() = default;

    void setNodeCount(int n);
    void setCellCount(int n);
    void setFaceCount(int n);
    void setXyzForNodes(std::function<void(int, double*)> get_node);
    void setComplexGetter(std::function<void(int,double*)> get_node);
    void setGlobalNodeIds(std::function<long(int)> get_global_node_id);
    void setOwningRankForNodes(std::function<int(int)> get_owning_rank_for_node);
    void setBoundaryConditions(std::function<int(int)> get_boundary_condition,
                                std::function<int(int)> number_of_nodes_in_boundary_face);
    void setFaces(std::function<int(int)> number_of_nodes_in_boundary_face,
                  std::function<void(int, int*)> get_nodes_in_boundary_face);
    void setCells(std::function<int(int)> number_of_nodes_in_cell, std::function<void(int, int*)> get_nodes_in_cell);
    void setComponentIdsForNodes(std::function<int(int)> get_comp_id_f);
    void fixInvalidFun3DComponentIds(MessagePasser mp);
    void addFace(int face_id, int face_size, std::function<void(int, int*)> get_nodes_in_boundary_face);
    void addCell(int cell_id, int cell_size, std::function<void(int, int*)> get_nodes_in_cell);
    int nodeCount() const;
    int getAssociatedComponentId(int node_id) const;
    template<typename T>
    Parfait::Point<T> getNode(int node_id) const;
    void nodeCoordinate(int node_id,double* p) const;
    int numberOfBoundaryFaces() const;
    auto getFaceType(int id) const;
    int getFaceIdInType(int id) const;
    YOGA::BoundaryConditions getBoundaryCondition(int face_id) const;
    std::vector<int> getNodesInBoundaryFace(int face_id) const;
    int numberOfNodesInBoundaryFace(int id) const;
    int numberOfCells() const;
    int cellCount(CellType cell_type) const;
    auto getCellType(int cell_id) const;
    int getCellIdInType(int cell_id) const;
    void getNodesInCell(int cell_id, int* cell) const;
    const int* cell_ptr(int cell_id) const;
    int numberOfNodesInCell(int id) const;
    long globalNodeId(int node_id) const;
    int nodeOwner(int node_id) const;
    int getCellIdFromOriginalMesh(int cell_id) const;

  private:
    std::vector<Parfait::Point<double>> nodes;
    std::vector<long> global_node_ids;
    std::vector<int> node_owning_rank;
    std::vector<int> component_id_for_node;
    int ncells;
    std::vector<std::array<int, 4>> tets;
    std::vector<int> tet_cell_ids;
    std::vector<std::array<int, 5>> pyramids;
    std::vector<int> pyramid_cell_ids;
    std::vector<std::array<int, 6>> prisms;
    std::vector<int> prism_cell_ids;
    std::vector<std::array<int, 8>> hexs;
    std::vector<int> hex_cell_ids;
    std::vector<std::pair<CellType, int>> cell_types;
    int nfaces;
    std::vector<std::array<int, 3>> triangles;
    std::vector<int> triangle_face_ids;
    std::vector<YOGA::BoundaryConditions> triangle_bcs;
    std::vector<std::array<int, 4>> quads;
    std::vector<int> quad_face_ids;
    std::vector<YOGA::BoundaryConditions> quad_bcs;
    std::vector<std::pair<FaceType, int>> face_types;
    std::vector<bool> is_ghost_node;

    std::function<void(int,double*)> complex_getter = [](int,double*){};
};

template<typename T>
Parfait::Point<T> YogaMesh::getNode(int node_id) const {
    Parfait::Point<T> p;
    if(std::is_same<T,double>())
        nodeCoordinate(node_id,(double*)p.data());
    else
        complex_getter(node_id,(double*)p.data());
    return p;
}


}
