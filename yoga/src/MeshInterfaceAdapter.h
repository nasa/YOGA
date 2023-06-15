#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/Point.h>
#include <t-infinity/MeshInterface.h>
#include <vector>
#include "BoundaryConditions.h"

namespace YOGA {

class MeshInterfaceAdapter {
  public:
    MeshInterfaceAdapter(MessagePasser mp,
                         const inf::MeshInterface& mesh,
                         int component_id,
                         const std::vector<YOGA::BoundaryConditions>& boundary_conditions);
    int numberOfNodes() const;
    int numberOfCells() const;
    long getGlobalNodeId(int id) const;
    std::vector<int> extractNodeOwners(const inf::MeshInterface& mesh) const;
    std::vector<int> extractCellOwners(const inf::MeshInterface& mesh) const;
    void throwIfBadCellType(int id) const;
    int numberOfNodesInCell(int id) const;
    std::vector<int> getNodesInCell(int id) const;
    int frameworkCellId(int id) const;
    void getNodesInCell(int id, int* cell) const;
    Parfait::Point<double> getNode(int id) const;
    Parfait::Point<double> getPoint(int id) const;
    int cellOwner(int cell_id) const;
    int getNodeOwner(int id) const;
    int getAssociatedComponentId(int id) const;
    int numberOfBoundaryFaces() const;
    std::vector<int> getNodesInBoundaryFace(int tri_id) const;
    BoundaryConditions getBoundaryCondition(int id) const;
    static std::vector<int> createTriToCellMap(const inf::MeshInterface& mesh);
    static std::vector<int> createQuadToCellMap(const inf::MeshInterface& mesh);
    static std::vector<int> createVolumeCellToCellIdMap(const inf::MeshInterface& mesh);

  private:
    MessagePasser mp;
    const inf::MeshInterface& mesh;
    const int component_id;
    const std::vector<int> node_owning_rank;
    const std::vector<int> cell_owning_rank;
    const std::vector<long> global_node_ids;
    const std::vector<YOGA::BoundaryConditions> boundary_conditions;
    const std::vector<int> triToCellId;
    const std::vector<int> quadToCellId;
    const std::vector<int> volumeCellToFrameworkCellId;

    std::vector<long> createGlobalNodeIds();
    std::vector<int> calcOwningRanksInGlobalComm(const std::vector<int>& owner_in_comm);
    std::vector<long> calcNodesPerDomain();
    std::vector<int> calcRanksPerDomain();
    long calcMaxNodeId();
};

}
