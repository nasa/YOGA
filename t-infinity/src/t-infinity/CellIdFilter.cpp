#include "CellIdFilter.h"
#include <parfait/Throw.h>
#include "VectorFieldAdapter.h"

using namespace inf;

CellIdFilter::CellIdFilter(MPI_Comm comm,
                           std::shared_ptr<inf::MeshInterface> mesh,
                           const std::vector<int>& selected_cell_ids)
    : mp(comm),
      output_cell_ids_to_original_cell_ids(selected_cell_ids),
      output_node_ids_to_original_node_ids(
          newNodeIdsToOriginalNodeIds(*mesh.get(), selected_cell_ids)),
      output_mesh(std::make_shared<CellSelectedMesh>(
          comm, mesh, selected_cell_ids, output_node_ids_to_original_node_ids)) {}

CellIdFilter::CellIdFilter(MPI_Comm comm,
                           std::shared_ptr<MeshInterface> mesh,
                           std::shared_ptr<CellSelector> selector)
    : mp(comm),
      output_cell_ids_to_original_cell_ids(selectCells(*mesh, *selector)),
      output_node_ids_to_original_node_ids(
          newNodeIdsToOriginalNodeIds(*mesh, output_cell_ids_to_original_cell_ids)),
      output_mesh(std::make_shared<CellSelectedMesh>(
          comm, mesh, output_cell_ids_to_original_cell_ids, output_node_ids_to_original_node_ids)) {
}

std::shared_ptr<CellSelectedMesh> CellIdFilter::getMesh() const { return output_mesh; }
std::vector<int> CellIdFilter::newNodeIdsToOriginalNodeIds(
    inf::MeshInterface& mesh, const std::vector<int>& selected_cell_ids) {
    std::vector<bool> isNodeNeeded(mesh.nodeCount(), false);
    sanityCheckCellIds(selected_cell_ids);
    for (int cell_id : selected_cell_ids) {
        auto cell_type = mesh.cellType(cell_id);
        int cell_length = mesh.cellTypeLength(cell_type);
        std::vector<int> cell(cell_length);
        mesh.cell(cell_id, cell.data());
        for (int i = 0; i < cell_length; i++) {
            if (cell[i] < 0 or size_t(cell[i]) > isNodeNeeded.size())
                throw std::logic_error("Cell has invalid node: " + std::to_string(cell[i]));
            isNodeNeeded.at(cell[i]) = true;
        }
    }
    std::vector<int> ids;
    for (int i = 0; i < mesh.nodeCount(); ++i)
        if (isNodeNeeded[i]) ids.push_back(i);
    return ids;
}

std::shared_ptr<inf::FieldInterface> CellIdFilter::apply(
    std::shared_ptr<inf::FieldInterface> field) const {
    auto association = field->attribute(FieldAttributes::Association());
    auto node = FieldAttributes::Node();
    auto cell = FieldAttributes::Cell();

    if (field->attribute(FieldAttributes::DataType()) != FieldAttributes::Float64())
        throw std::logic_error("Attempted to filter a field that is not a double based field.");
    if (association == node) {
        return apply(field, output_node_ids_to_original_node_ids);
    } else if (association == cell) {
        return apply(field, output_cell_ids_to_original_cell_ids);
    } else {
        PARFAIT_THROW("CellId filter only works on NODE or CELL fields");
    }
}
std::shared_ptr<FieldInterface> CellIdFilter::apply(
    const std::shared_ptr<FieldInterface>& field,
    const std::vector<int>& output_to_original_ids) const {
    std::vector<double> sampled_data(output_to_original_ids.size() * field->blockSize());
    std::vector<double> value(field->blockSize());
    for (size_t index = 0; index < output_to_original_ids.size(); index++) {
        int cell_id = output_to_original_ids[index];
        field->value(cell_id, value.data());
        for (int i = 0; i < field->blockSize(); i++) {
            sampled_data[field->blockSize() * index + i] = value[i];
        }
    }
    auto out = std::make_shared<VectorFieldAdapter>(field->attribute("name"),
                                                    field->attribute("association"),
                                                    field->blockSize(),
                                                    sampled_data);
    auto attributes = field->getAllAttributes();
    for (auto& pair : attributes) {
        auto key = pair.first;
        auto value = pair.second;
        out->setAdapterAttribute(key, value);
    }
    return out;
}

void CellIdFilter::sanityCheckCellIds(const std::vector<int>& selected_cell_ids) {
    for (auto cid : selected_cell_ids) {
        if (cid < 0)
            throw std::logic_error(std::string(__FILE__) + std::string(__FUNCTION__) +
                                   " requested cell ID is negative.");
    }
}
std::vector<int> CellIdFilter::selectCells(const MeshInterface& mesh,
                                           const CellSelector& selector) const {
    std::vector<int> ids;
    for (int i = 0; i < mesh.cellCount(); i++)
        if (selector.keep(mesh, i)) ids.push_back(i);
    return ids;
}
