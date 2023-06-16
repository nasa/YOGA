#include "NaiveMesh.h"

void NaiveMesh::cell(inf::MeshInterface::CellType type, int cell_id, long* cell_out) const {
    auto length = inf::MeshInterface::cellTypeLength(type);
    for (int i = 0; i < length; i++) {
        cell_out[i] = cells.at(type)[length * cell_id + i];
    }
}

bool NaiveMesh::doOwnNode(long global) const { return global >= node_range.start and global < node_range.end; }

int NaiveMesh::ownedCount() const { return node_range.end - node_range.start; }

std::vector<inf::MeshInterface::CellType> NaiveMesh::cellTypes() const {
    std::vector<inf::MeshInterface::CellType> cell_types;
    for (auto& pair : cells) {
        cell_types.push_back(pair.first);
    }
    return cell_types;
}

NaiveMesh::NaiveMesh(const std::vector<Parfait::Point<double>>& coords,
                     const std::map<inf::MeshInterface::CellType, std::vector<long>>& cells,
                     const std::map<inf::MeshInterface::CellType, std::vector<int>>& cell_tags,
                     const std::map<inf::MeshInterface::CellType, std::vector<long>>& global_cell_ids,
                     const std::vector<long>& global_node_id,
                     const Parfait::LinearPartitioner::Range<long>& node_range)
    : xyz(coords),
      cells(cells),
      cell_tags(cell_tags),
      global_cell_ids(global_cell_ids),
      local_to_global_node(global_node_id),
      node_range(node_range) {
    for (int local = 0; local < long(local_to_global_node.size()); local++) {
        auto global = local_to_global_node[local];
        global_to_local_node[global] = local;
    }
}

long NaiveMesh::cellCount(inf::MeshInterface::CellType cell_type) const {
    auto cell_length = inf::MeshInterface::cellTypeLength(cell_type);
    if (cells.count(cell_type) != 0)
        return cells.at(cell_type).size() / cell_length;
    else
        return 0;
}
