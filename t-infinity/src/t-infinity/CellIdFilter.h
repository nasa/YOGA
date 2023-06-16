#pragma once
#include <t-infinity/MeshInterface.h>
#include <vector>
#include "CellSelectedMesh.h"
#include "CellSelector.h"

namespace inf {
class CellIdFilter {
  public:
    CellIdFilter(MPI_Comm comm,
                 std::shared_ptr<inf::MeshInterface> mesh,
                 const std::vector<int>& selected_cell_ids);
    CellIdFilter(MPI_Comm comm,
                 std::shared_ptr<inf::MeshInterface> mesh,
                 std::shared_ptr<CellSelector> selector);
    std::shared_ptr<inf::CellSelectedMesh> getMesh() const;
    std::shared_ptr<inf::FieldInterface> apply(std::shared_ptr<inf::FieldInterface> field) const;

  private:
    MessagePasser mp;
    std::vector<int> output_cell_ids_to_original_cell_ids;
    std::vector<int> output_node_ids_to_original_node_ids;
    std::shared_ptr<inf::CellSelectedMesh> output_mesh;

    std::vector<int> newNodeIdsToOriginalNodeIds(inf::MeshInterface& mesh,
                                                 const std::vector<int>& selected_cell_ids);
    void sanityCheckCellIds(const std::vector<int>& selected_cell_ids);
    std::shared_ptr<FieldInterface> apply(const std::shared_ptr<FieldInterface>& field,
                                          const std::vector<int>& output_to_original_ids) const;
    std::vector<int> selectCells(const MeshInterface& mesh, const CellSelector& selector) const;
};
}
