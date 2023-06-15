#include <t-infinity/MeshInterface.h>
#include <parfait/Throw.h>
#include <parfait/ToString.h>
#include "CellSelectedMesh.h"
#include <utility>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

namespace inf {

std::vector<long> mapNewGlobalIdsFromSparseSelection(MessagePasser mp,
                                                     const std::vector<long>& selection,
                                                     const std::vector<int>& owner) {
    if (selection.size() != owner.size()) {
        PARFAIT_THROW("Array size sanity check failed.  Must have an owner for each selection\n");
    }
    for (auto o : owner) {
        if (o < 0 or o >= mp.NumberOfProcesses()) {
            PARFAIT_THROW("An input owner makes no sense: " + std::to_string(o));
        }
    }

    std::map<int, std::vector<long>> i_need_these_from_these_ranks;
    long num_i_assign = 0;
    for (size_t i = 0; i < selection.size(); i++) {
        if (owner[i] == mp.Rank()) {
            num_i_assign++;
        } else {
            i_need_these_from_these_ranks[owner[i]].push_back(selection[i]);
        }
    }

    auto num_each_rank_owns = mp.Gather(num_i_assign);
    long my_offset = 0;
    for (int rank = 0; rank < mp.Rank(); rank++) {
        my_offset += num_each_rank_owns[rank];
    }

    std::unordered_map<long, long> my_assigned_old_to_new;

    long next_gid = my_offset;
    for (size_t i = 0; i < selection.size(); i++) {
        if (owner[i] == mp.Rank()) {
            long old_gid = selection[i];
            long new_gid = next_gid++;
            my_assigned_old_to_new[old_gid] = new_gid;
        }
    }

    auto requests_for_gids = mp.Exchange(i_need_these_from_these_ranks);
    for (auto& pair : requests_for_gids) {
        auto& gids = pair.second;
        for (auto& g : gids) {
            g = my_assigned_old_to_new.at(g);
        }
    }

    auto new_gids_i_requested = mp.Exchange(requests_for_gids);

    for (auto& pair : new_gids_i_requested) {
        auto& rank = pair.first;
        auto& new_gids = pair.second;
        for (size_t i = 0; i < new_gids.size(); i++) {
            auto old_gid = i_need_these_from_these_ranks.at(rank).at(i);
            auto new_gid = new_gids.at(i);
            my_assigned_old_to_new[old_gid] = new_gid;
        }
    }

    std::vector<long> new_gids_in_order_mathing_input_gids(selection.size());
    for (size_t i = 0; i < selection.size(); i++) {
        auto new_gid = my_assigned_old_to_new.at(selection[i]);
        new_gids_in_order_mathing_input_gids[i] = new_gid;
    }

    return new_gids_in_order_mathing_input_gids;
}
CellSelectedMesh::CellSelectedMesh(MPI_Comm comm,
                                   std::shared_ptr<MeshInterface> m,
                                   const std::vector<int>& selected_cell_ids,
                                   const std::vector<int>& selected_node_ids)
    : mp(comm),
      mesh(std::move(m)),
      cell_ids_selection_to_original(selected_cell_ids),
      node_ids_selection_to_original(selected_node_ids),
      cell_counts(countCellsOfEachType(cell_ids_selection_to_original)),
      originalNodeIdsToSelection(mapOriginalNodeIdsToSelection(node_ids_selection_to_original)),
      global_node_ids(assignNewGlobalNodeIds()),
      global_cell_ids(assignNewGlobalCellIds()) {}

int CellSelectedMesh::nodeCount() const { return node_ids_selection_to_original.size(); }
int CellSelectedMesh::cellCount() const { return cell_ids_selection_to_original.size(); }
int CellSelectedMesh::cellCount(inf::MeshInterface::CellType cell_type) const {
    if (cell_counts.count(cell_type) == 0)
        return 0;
    else
        return cell_counts.at(cell_type);
}
int CellSelectedMesh::cellTag(int cell_id) const {
    int tag;
    try {
        tag = mesh->cellTag(cell_ids_selection_to_original.at(cell_id));
    } catch (...) {
        printf("Couldn't find original cell id for selected cell %d", cell_id);
        fflush(stdout);
        throw;
    }
    return tag;
}
long CellSelectedMesh::globalNodeId(int id) const { return global_node_ids.at(id); }
long CellSelectedMesh::globalCellId(int id) const { return global_cell_ids.at(id); }

inf::MeshInterface::CellType CellSelectedMesh::cellType(int cell_id) const {
    return mesh->cellType(cell_ids_selection_to_original.at(cell_id));
}
int CellSelectedMesh::nodeOwner(int id) const {
    return mesh->nodeOwner(node_ids_selection_to_original.at(id));
}
int CellSelectedMesh::cellOwner(int id) const {
    return mesh->cellOwner(cell_ids_selection_to_original.at(id));
}

void CellSelectedMesh::nodeCoordinate(int node_id, double* coord) const {
    mesh->nodeCoordinate(node_ids_selection_to_original.at(node_id), coord);
}
void CellSelectedMesh::cell(int cell_id, int* cell_out) const {
    mesh->cell(cell_ids_selection_to_original.at(cell_id), cell_out);
    auto cell_type = cellType(cell_id);
    int cell_length = cellTypeLength(cell_type);
    for (int i = 0; i < cell_length; i++) {
        cell_out[i] = originalNodeIdsToSelection.at(cell_out[i]);
    }
}
std::vector<int> CellSelectedMesh::selectToMeshIds() const {
    return node_ids_selection_to_original;
}
int CellSelectedMesh::previousNodeId(int node_id) const {
    return node_ids_selection_to_original[node_id];
}
int CellSelectedMesh::previousCellId(int cell_id) const {
    return cell_ids_selection_to_original[cell_id];
}
std::map<inf::MeshInterface::CellType, int> CellSelectedMesh::countCellsOfEachType(
    const std::vector<int>& cell_ids) const {
    std::map<MeshInterface::CellType, int> cell_count_map;
    for (int id : cell_ids) {
        auto type = mesh->cellType(id);
        if (cell_count_map.count(type) > 0)
            cell_count_map[type]++;
        else
            cell_count_map[type] = 1;
    }
    return cell_count_map;
}
std::map<int, int> CellSelectedMesh::mapOriginalNodeIdsToSelection(const std::vector<int>& nodes) {
    std::map<int, int> map;
    for (int i = 0; i < int(nodes.size()); ++i) map[nodes[i]] = i;
    return map;
}
std::vector<long> CellSelectedMesh::assignNewGlobalNodeIds() {
    std::vector<int> owner(node_ids_selection_to_original.size());
    auto to_owner = [&](int local) { return mesh->nodeOwner(local); };
    std::transform(node_ids_selection_to_original.begin(),
                   node_ids_selection_to_original.end(),
                   owner.begin(),
                   to_owner);

    auto to_global = [&](int local) { return mesh->globalNodeId(local); };
    std::vector<long> selection_global_ids(node_ids_selection_to_original.size());
    std::transform(node_ids_selection_to_original.begin(),
                   node_ids_selection_to_original.end(),
                   selection_global_ids.begin(),
                   to_global);
    return mapNewGlobalIdsFromSparseSelection(mp, selection_global_ids, owner);
}
std::vector<long> CellSelectedMesh::assignNewGlobalCellIds() {
    std::vector<int> owner(cell_ids_selection_to_original.size());
    auto to_owner = [&](int local) { return mesh->cellOwner(local); };
    std::transform(cell_ids_selection_to_original.begin(),
                   cell_ids_selection_to_original.end(),
                   owner.begin(),
                   to_owner);

    std::vector<long> selection_global_ids(cell_ids_selection_to_original.size());
    auto to_global = [&](int local) { return mesh->globalCellId(local); };
    std::transform(cell_ids_selection_to_original.begin(),
                   cell_ids_selection_to_original.end(),
                   selection_global_ids.begin(),
                   to_global);
    return mapNewGlobalIdsFromSparseSelection(mp, selection_global_ids, owner);
}
int CellSelectedMesh::partitionId() const { return mesh->partitionId(); }
std::string CellSelectedMesh::tagName(int t) const { return mesh->tagName(t); }
}
