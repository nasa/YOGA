#pragma once
#include <t-infinity/MeshInterface.h>
#include <mpi.h>
#include <parfait/Extent.h>
#include <parfait/ExtentBuilder.h>
#include <parfait/Point.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>
#include <t-infinity/FieldInterface.h>
#include <MessagePasser/MessagePasser.h>

namespace inf {
class CellSelectedMesh : public inf::MeshInterface {
  public:
    CellSelectedMesh(MPI_Comm comm,
                     std::shared_ptr<inf::MeshInterface> m,
                     const std::vector<int>& selected_cell_ids,
                     const std::vector<int>& selected_node_ids);

    int partitionId() const override;
    int nodeCount() const override;
    int cellCount() const override;
    int cellCount(CellType cell_type) const override;
    int cellTag(int cell_id) const override;
    long globalNodeId(int id) const override;
    long globalCellId(int id) const override;
    CellType cellType(int cell_id) const override;
    int nodeOwner(int id) const override;
    int cellOwner(int id) const override;
    void nodeCoordinate(int node_id, double* coord) const override;
    void cell(int cell_id, int* cell_out) const override;
    std::vector<int> selectToMeshIds() const;
    int previousNodeId(int node_id) const;
    int previousCellId(int cell_id) const;
    std::string tagName(int t) const override;

  private:
    MessagePasser mp;
    std::shared_ptr<MeshInterface> mesh;
    std::vector<int> cell_ids_selection_to_original;
    std::vector<int> node_ids_selection_to_original;
    std::map<MeshInterface::CellType, int> cell_counts;
    std::map<int, int> originalNodeIdsToSelection;
    std::vector<long> global_node_ids;
    std::vector<long> global_cell_ids;

    std::map<MeshInterface::CellType, int> countCellsOfEachType(
        const std::vector<int>& cell_ids) const;

    std::map<int, int> mapOriginalNodeIdsToSelection(const std::vector<int>& nodes);
    std::vector<long> assignNewGlobalNodeIds();
    std::vector<long> assignNewGlobalCellIds();
};

std::vector<long> mapNewGlobalIdsFromSparseSelection(MessagePasser mp,
                                                     const std::vector<long>& selection,
                                                     const std::vector<int>& owner);
}
