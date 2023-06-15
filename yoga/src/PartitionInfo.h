#pragma once

#include <parfait/Extent.h>
#include <parfait/ExtentBuilder.h>
#include <map>
#include <set>
#include "BoundaryConditions.h"
#include "YogaMesh.h"

namespace YOGA {

class PartitionInfo {
  public:
    PartitionInfo(const YogaMesh& mesh, int rank);

    int numberOfBodies() const;
    int numberOfComponentMeshes() const;
    bool containsBody(int id) const;
    bool containsComponentMesh(int id) const;
    std::vector<int> getBodyIds() const;
    std::vector<int> getComponentIds() const;

    Parfait::Extent<double> getExtentForBody(int i) const;
    Parfait::Extent<double> getExtentForComponent(int i) const;

    int getAssociatedComponentIdForFace(int i) const;
    Parfait::Extent<double> getExtentForFace(int i) const;
    int getAssociatedComponentIdForCell(int i) const;
    bool isCellMine(int i) const;
    bool isDonorCandidate(int i) const;
    const Parfait::Extent<double>& getExtentForCell(int i) const;
    int numberOfFaces() const { return face_extents.size(); }
    int numberOfCells() const { return cell_extents.size(); }
    BoundaryConditions getBoundaryConditionForNode(int i) const { return boundaryConditionsForNodes[i]; }
    BoundaryConditions getBoundaryConditionForFace(int i) { return boundaryConditionsForFaces[i]; }

    static std::vector<BoundaryConditions> createNodeBcs(const YogaMesh& mesh);
  private:
    std::map<int, Parfait::Extent<double>> body_extents;
    std::map<int, Parfait::Extent<double>> component_extents;
    std::vector<int> face_associated_component_id;
    std::vector<Parfait::Extent<double>> face_extents;
    std::vector<int> cell_associated_component_id;
    std::vector<Parfait::Extent<double>> cell_extents;
    std::vector<bool> i_own_cell;
    std::vector<BoundaryConditions> boundaryConditionsForNodes;
    std::vector<BoundaryConditions> boundaryConditionsForFaces;
    std::vector<bool> is_donor_candidate;

    std::vector<bool> determineDonorCandidacy(const YogaMesh& mesh,
                                              const std::vector<YOGA::BoundaryConditions>& node_bcs);
    std::vector<BoundaryConditions> createFaceBcs(const YogaMesh& mesh);
    void createCellOwnership(const YogaMesh& mesh, int rank);
    void addFaceExtentToBodyExtent(const YogaMesh& mesh, int face_id);
    void addNodeToComponentExtent(const YogaMesh& mesh, int i);
    void buildFaceComponentList(const YogaMesh& mesh);
    void buildFaceExtents(const YogaMesh& mesh);
    void buildCellComponentList(const YogaMesh& mesh);
    void buildCellExtents(const YogaMesh& mesh);
    void throwIfBodyNotFound(int i) const;
    void throwIfComponentNotFound(int i) const;
};
}
