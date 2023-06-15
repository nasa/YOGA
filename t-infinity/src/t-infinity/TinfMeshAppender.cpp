#include "TinfMeshAppender.h"
#include <parfait/Throw.h>

std::pair<inf::MeshInterface::CellType, int> TinfMeshAppender::addCell(
    const inf::TinfMeshCell& cell) {
    if (resident_global_cell_ids.count(cell.global_id) == 0) {
        return appendCell(cell);
    } else {
        throwIfIncomingCellIsDifferent(cell);
        return {};
    }
}
inf::TinfMeshData& TinfMeshAppender::getData() { return mesh_data; }
const inf::TinfMeshData& TinfMeshAppender::getData() const { return mesh_data; }
int TinfMeshAppender::cellCount() const { return int(resident_global_cell_ids.size()); }
std::pair<inf::MeshInterface::CellType, int> TinfMeshAppender::appendCell(
    const inf::TinfMeshCell& cell) {
    for (size_t i = 0; i < cell.nodes.size(); i++) {
        long global_node_id = cell.nodes[i];
        if (node_g2l.count(global_node_id) == 0) {
            node_g2l[global_node_id] = int(mesh_data.points.size());
            mesh_data.points.push_back(cell.points[i]);
            mesh_data.node_owner.push_back(cell.node_owner[i]);
            mesh_data.global_node_id.push_back(global_node_id);
        }
    }
    resident_global_cell_ids.insert(cell.global_id);
    mesh_data.cell_tags[cell.type].push_back(cell.tag);
    mesh_data.global_cell_id[cell.type].push_back(cell.global_id);
    mesh_data.cell_owner[cell.type].push_back(cell.owner);
    for (auto& gid : cell.nodes) {
        mesh_data.cells[cell.type].push_back(node_g2l.at(gid));
    }

    int index = mesh_data.cell_tags.at(cell.type).size() - 1;
    global_cell_id_to_key[cell.global_id] = {cell.type, index};
    return {cell.type, index};
}
void TinfMeshAppender::throwIfIncomingCellIsDifferent(const inf::TinfMeshCell& cell) {
    inf::MeshInterface::CellType type;
    int index;
    std::tie(type, index) = global_cell_id_to_key.at(cell.global_id);
    auto my_cell = mesh_data.getCell(type, index);
    bool bad = false;
    bad |= my_cell.nodes.size() != cell.nodes.size();
    for (size_t i = 0; i < cell.nodes.size(); i++) bad |= my_cell.nodes[i] != cell.nodes[i];

    if (bad)
        PARFAIT_THROW(
            "Incoming cell is meaningfully different than the cell I have already for that global "
            "cell id");
}
