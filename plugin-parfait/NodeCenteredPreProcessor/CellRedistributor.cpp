#include "CellRedistributor.h"
#include <algorithm>
#include <set>
#include "Tracer.h"

CellRedistributor::CellRedistributor(MessagePasser mp,
                                     const std::map<inf::MeshInterface::CellType, std::vector<long>>& cells,
                                     const std::map<inf::MeshInterface::CellType, std::vector<int>>& cell_tags,
                                     const std::map<inf::MeshInterface::CellType, std::vector<long>>& global_cell_ids)
    : mp(mp), cells(cells), cell_tags(cell_tags), global_cell_ids(global_cell_ids) {}

std::vector<int> CellRedistributor::determineWhereToSendCell(std::function<bool(long)> responsibleForNode,
                                                             std::function<int(long)> getNodeFutureOwner,
                                                             const long* cell,
                                                             int length) {
    std::set<int> send_cell_to_these_ranks;
    for (int i = 0; i < length; i++) {
        long gid = cell[i];
        if (responsibleForNode(gid)) {
            int future_owner = getNodeFutureOwner(gid);
            send_cell_to_these_ranks.insert(future_owner);
        }
    }
    return std::vector<int>(send_cell_to_these_ranks.begin(), send_cell_to_these_ranks.end());
}
std::vector<std::vector<std::pair<inf::MeshInterface::CellType, int>>> CellRedistributor::assignCellKeysToRanks(
    std::function<bool(long)> amIResponsibleForNode, std::function<int(long)> getNodeFutureOwner) {
    std::vector<std::vector<std::pair<inf::MeshInterface::CellType, int>>> cells_for_ranks(mp.NumberOfProcesses());
    for (auto& pair : cells) {
        auto type = pair.first;
        int cell_length = inf::MeshInterface::cellTypeLength(type);
        auto& cells_in_type = pair.second;
        auto num_cells = cells_in_type.size() / cell_length;
        for (unsigned int cell_id = 0; cell_id < num_cells; cell_id++) {
            auto send_this_cell_to_these_ranks = determineWhereToSendCell(
                amIResponsibleForNode, getNodeFutureOwner, &cells_in_type[cell_id * cell_length], cell_length);
            for (auto r : send_this_cell_to_these_ranks) cells_for_ranks[r].push_back({type, cell_id});
        }
    }
    return cells_for_ranks;
}

std::vector<std::pair<inf::MeshInterface::CellType, int>> CellRedistributor::queueCellsContainingNodes(
    std::vector<long>& nodes) {
    std::sort(nodes.begin(), nodes.end());
    std::vector<std::pair<inf::MeshInterface::CellType, int>> queued_cells;
    for (auto& pair : cells) {
        auto type = pair.first;
        int cell_length = inf::MeshInterface::cellTypeLength(type);
        auto& cells_in_type = pair.second;
        auto num_cells = cells_in_type.size() / cell_length;
        for (unsigned int cell_id = 0; cell_id < num_cells; cell_id++) {
            if (shouldSendCell(nodes, &cells_in_type[cell_id * cell_length], cell_length)) {
                queued_cells.emplace_back(pair.first, cell_id);
            }
        }
    }
    return queued_cells;
}

bool CellRedistributor::shouldSendCell(std::vector<long>& needed_nodes, const long* cell, int cell_length) const {
    for (int i = 0; i < cell_length; i++) {
        long global_id = cell[i];
        if (std::binary_search(needed_nodes.begin(), needed_nodes.end(), global_id)) return true;
    }
    return false;
}

std::vector<long> CellRedistributor::packCells(
    const std::vector<std::pair<inf::MeshInterface::CellType, int>>& send_cells) {
    std::vector<long> packed;
    for (auto& pair : send_cells) {
        inf::MeshInterface::CellType cell_type = pair.first;
        int cell_id = pair.second;
        int cell_length = inf::MeshInterface::cellTypeLength(cell_type);
        packed.push_back(long(cell_type));
        packed.push_back(long(cell_tags.at(cell_type)[cell_id]));
        packed.push_back(long(global_cell_ids.at(cell_type)[cell_id]));
        for (int i = 0; i < cell_length; i++) {
            packed.push_back(cells.at(cell_type)[cell_length * cell_id + i]);
        }
    }
    return packed;
}

CellCollection CellRedistributor::unpackCells(std::vector<std::vector<long>>&& packed_cells_from_other_ranks) {
    CellCollection collection;
    std::set<long> already_unpacked_global_cell_ids;
    for (auto& packed_cells : packed_cells_from_other_ranks) {
        for (size_t index = 0; index < packed_cells.size();) {
            auto cell_type = inf::MeshInterface::CellType(packed_cells[index++]);
            long tag = packed_cells[index++];
            int cell_length = inf::MeshInterface::cellTypeLength(cell_type);
            long gid = packed_cells[index++];

            if (already_unpacked_global_cell_ids.count(gid) == 0) {
                already_unpacked_global_cell_ids.insert(gid);
                collection.cell_tags[cell_type].push_back(tag);
                collection.cell_global_ids[cell_type].push_back(gid);
                for (int i = 0; i < cell_length; i++) collection.cells[cell_type].push_back(packed_cells[index++]);
            } else {
                index += cell_length;  // skip over cell if we already unpacked it from someone else
            }
        }
        packed_cells.clear();
        packed_cells.shrink_to_fit();
    }
    return collection;
}

CellCollection CellRedistributor::redistributeCells(std::function<bool(long)> amIResponsibleForNode,
                                                    std::function<long(int)> getGlobalNodeId,
                                                    const std::vector<int>& future_node_owners) {
    Tracer::begin(__FUNCTION__);
    Tracer::traceMemory();

    Tracer::begin("build g2l");
    std::map<long, int> global_to_local;
    for (int local = 0; local < long(future_node_owners.size()); local++) {
        global_to_local[getGlobalNodeId(local)] = local;
    }
    Tracer::traceMemory();
    Tracer::end("build g2l");

    auto getNodeFutureOwner = [&](long global) {
        auto local = global_to_local.at(global);
        return future_node_owners[local];
    };
    Tracer::begin("assign cell keys");
    std::vector<std::vector<std::pair<inf::MeshInterface::CellType, int>>> cell_keys_for_ranks =
        assignCellKeysToRanks(amIResponsibleForNode, getNodeFutureOwner);
    global_to_local.clear();
    Tracer::traceMemory();
    Tracer::end("assign cell keys");

    Tracer::begin("pack cells");
    std::vector<std::vector<long>> packed_cells_for_ranks(mp.NumberOfProcesses());
    for (int r = 0; r < mp.NumberOfProcesses(); r++) {
        packed_cells_for_ranks[r] = packCells(cell_keys_for_ranks[r]);
    }
    Tracer::traceMemory();
    Tracer::end("pack cells");
    cell_keys_for_ranks.clear();
    cell_keys_for_ranks.shrink_to_fit();

    Tracer::begin("exchange");
    auto e = mp.Exchange(std::move(packed_cells_for_ranks));
    Tracer::traceMemory();
    Tracer::end("exchange");

    Tracer::begin("unpack");
    CellCollection collection = unpackCells(std::move(e));
    Tracer::traceMemory();
    Tracer::end("unpack");
    Tracer::end(__FUNCTION__);
    return collection;
}
