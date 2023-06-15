#include "Redistributor.h"
#include <parfait/ToString.h>
#include <algorithm>
#include <set>
#include "../shared/GlobalCellIdBuilder.h"
#include "CellRedistributor.h"
#include "InitialNodeSyncer.h"

std::shared_ptr<inf::TinfMesh> Redistributor::createMesh() {
    mp_rootprint("Redistribute nodes\n");
    auto owned_global_nodes = redistributeNodes();
    mp_rootprint("Redistribute cells\n");
    auto cell_collection = redistributeCells(owned_global_nodes);
    auto resident_global_nodes = determineResidentNodes(cell_collection.cells);
    auto global_to_local = buildGlobalToLocal(resident_global_nodes);
    std::vector<Parfait::Point<double>> points;
    std::vector<int> node_owner;
    std::tie(points, node_owner) = gatherAllResidentCoordinates(resident_global_nodes);

    mp_rootprint("Map cells to local\n");
    inf::TinfMeshData data;
    data.cells = mapCellsToLocal(cell_collection.cells, global_to_local);
    data.cell_tags = cell_collection.cell_tags;
    data.global_cell_id = cell_collection.cell_global_ids;
    cell_collection.cells.clear();
    cell_collection.cell_tags.clear();
    cell_collection.cell_global_ids.clear();
    data.global_node_id = resident_global_nodes;
    data.points = points;
    data.node_owner = node_owner;
    points.clear();
    points.shrink_to_fit();
    node_owner.clear();
    node_owner.shrink_to_fit();
    data.cell_owner = determineCellOwner(data.cells, data.global_node_id, data.node_owner);

    mp_rootprint("Make TinfMesh\n");
    auto ptr = std::make_shared<inf::TinfMesh>(std::move(data), mp.Rank());
    return ptr;
}

std::map<inf::MeshInterface::CellType, std::vector<int>> Redistributor::mapCellsToLocal(
    const std::map<inf::MeshInterface::CellType, std::vector<long>>& cells_global,
    const std::map<long, int>& global_to_local) const {
    std::map<inf::MeshInterface::CellType, std::vector<int>> cells;
    for (auto& pair : cells_global) {
        auto cell_type = pair.first;
        for (long i : pair.second) cells[cell_type].push_back(global_to_local.at(i));
    }
    return cells;
}

std::vector<long> Redistributor::redistributeNodes() {
    InitialNodeSyncer nodeSyncer(mp);
    std::vector<long> global_node_ids;
    for (size_t n = 0; n < partition_vector.size(); n++) {
        global_node_ids.push_back(naive_mesh.local_to_global_node.at(n));
    }
    return nodeSyncer.syncOwnedGlobalNodes(global_node_ids, partition_vector);
}

CellCollection Redistributor::redistributeCells(const std::vector<long>& global_nodes) {
    CellRedistributor cellRedistributor(mp, naive_mesh.cells, naive_mesh.cell_tags, naive_mesh.global_cell_ids);
    auto get_global_node_id = [&](int id) { return naive_mesh.local_to_global_node.at(id); };
    auto amIResponsibleForNode = [&](long gid) { return naive_mesh.doOwnNode(gid); };
    return cellRedistributor.redistributeCells(amIResponsibleForNode, get_global_node_id, partition_vector);
}

std::vector<int> Redistributor::determineNodeOwners(MessagePasser mp,
                                                    long max_owned_id,
                                                    const std::vector<long>& global_node_ids) {
    std::vector<long> ranges;
    mp.Gather(max_owned_id, ranges);

    std::vector<int> owners(global_node_ids.size());
    for (size_t i = 0; i < global_node_ids.size(); i++) {
        owners[i] = std::upper_bound(ranges.begin(), ranges.end(), global_node_ids[i]) - ranges.begin();
    }

    return owners;
}

std::vector<long> Redistributor::determineResidentNodes(
    const std::map<inf::MeshInterface::CellType, std::vector<long>>& cells) {
    std::set<long> resident_nodes;

    for (auto& pair : cells) {
        for (auto global_id : pair.second) resident_nodes.insert(global_id);
    }
    return {resident_nodes.begin(), resident_nodes.end()};
}

void Redistributor::fulfillRequest(const std::vector<long>& requested_gids,
                                   std::vector<long>& gids,
                                   std::vector<Parfait::Point<double>>& points,
                                   std::vector<int>& node_owners) {
    gids.clear();
    points.clear();
    node_owners.clear();
    for (auto global : requested_gids) {
        if (naive_mesh.doOwnNode(global)) {
            gids.push_back(global);
            int local = naive_mesh.global_to_local_node.at(global);
            points.push_back(naive_mesh.xyz[local]);
            node_owners.push_back(partition_vector[local]);
        }
    }
}

std::pair<std::vector<Parfait::Point<double>>, std::vector<int>> Redistributor::gatherAllResidentCoordinates(
    const std::vector<long>& resident_node_ids) {
    std::map<long, int> global_to_local = buildGlobalToLocal(resident_node_ids);

    long global_node_count = 0;
    for (long gid : resident_node_ids) global_node_count = std::max(gid, global_node_count);
    global_node_count = mp.ParallelMax(global_node_count) + 1;
    std::vector<std::vector<long>> need(mp.NumberOfProcesses());
    for (long gid : resident_node_ids) {
        long original_owner =
            Parfait::LinearPartitioner::getWorkerOfWorkItem(gid, global_node_count, mp.NumberOfProcesses());
        need[original_owner].push_back(gid);
    }

    auto requested_gids = mp.Exchange(need);

    std::vector<std::vector<Parfait::Point<double>>> reply_points(mp.NumberOfProcesses());
    std::vector<std::vector<long>> reply_gids(mp.NumberOfProcesses());
    std::vector<std::vector<int>> reply_owners(mp.NumberOfProcesses());
    for (int r = 0; r < mp.NumberOfProcesses(); r++) {
        std::vector<long> send_ids;
        std::vector<Parfait::Point<double>> send_points;
        std::vector<int> send_owner;

        fulfillRequest(requested_gids[r], send_ids, send_points, send_owner);
        reply_gids[r].insert(reply_gids[r].end(), send_ids.begin(), send_ids.end());
        reply_points[r].insert(reply_points[r].end(), send_points.begin(), send_points.end());
        reply_owners[r].insert(reply_owners[r].end(), send_owner.begin(), send_owner.end());
    }

    auto ids_from_ranks = mp.Exchange(reply_gids);
    auto points_from_ranks = mp.Exchange(reply_points);
    auto owners_from_ranks = mp.Exchange(reply_owners);

    std::vector<Parfait::Point<double>> output_coords(resident_node_ids.size());
    std::vector<int> output_owner(resident_node_ids.size());

    for (int r = 0; r < mp.NumberOfProcesses(); r++) {
        auto& ids = ids_from_ranks[r];
        auto& points = points_from_ranks[r];
        auto& owners = owners_from_ranks[r];
        for (size_t i = 0; i < ids.size(); i++) {
            int local = global_to_local.at(ids[i]);
            output_coords[local] = points[i];
            output_owner[local] = owners[i];
        }
    }

    return {output_coords, output_owner};
}

std::map<long, int> Redistributor::buildGlobalToLocal(const std::vector<long>& resident_node_ids) const {
    std::map<long, int> global_to_local;
    int local = 0;
    for (auto l : resident_node_ids) global_to_local[l] = local++;
    return global_to_local;
}

std::pair<std::vector<int>, inf::MeshInterface::CellType> extractCellAndType(
    const std::map<inf::MeshInterface::CellType, std::vector<int>>& cells, int cell_id) {
    for (auto& pair : cells) {
        auto type = pair.first;
        const auto& type_collection = pair.second;
        int type_length = inf::MeshInterface::cellTypeLength(type);
        int num_cells_in_type = type_collection.size() / type_length;
        if (cell_id < num_cells_in_type) {
            std::vector<int> this_cell(type_length);
            for (int i = 0; i < long(this_cell.size()); i++) {
                this_cell[i] = type_collection[type_length * cell_id + i];
            }
            return std::make_pair(this_cell, type);
        } else {
            cell_id -= num_cells_in_type;
        }
    }
    throw std::logic_error("Couldn't get cell: " + std::to_string(cell_id) + " " + std::string(__FILE__));
}

std::map<inf::MeshInterface::CellType, std::vector<int>> Redistributor::determineCellOwner(
    const std::map<inf::MeshInterface::CellType, std::vector<int>>& cells,
    const std::vector<long>& local_to_global_node_ids,
    const std::vector<int>& node_owners) {
    int cell_count = 0;
    for (auto& coll : cells) {
        auto type = coll.first;
        int length = inf::MeshInterface::cellTypeLength(type);
        cell_count += coll.second.size() / length;
    }

    std::map<inf::MeshInterface::CellType, std::vector<int>> owners;
    for (int cell_id = 0; cell_id < cell_count; cell_id++) {
        auto cell_and_type = extractCellAndType(cells, cell_id);
        auto elem = cell_and_type.first;
        auto cell_type = cell_and_type.second;
        auto elem_global = getGlobalNodeIdsForCell(local_to_global_node_ids, elem);
        auto cell_node_owners = getCellNodeOwners(elem, node_owners);
        int owner =
            Parfait::Deprecated::GlobalCellIdBuilder::inferCellOwnerFromNodeOwners(elem_global, cell_node_owners);
        owners[cell_type].push_back(owner);
    }
    return owners;
}
std::vector<long> Redistributor::getGlobalNodeIdsForCell(const std::vector<long>& global_to_local_node_ids,
                                                         const std::vector<int>& cell) {
    std::vector<long> cell_in_global_node_ids(cell.size());
    for (size_t i = 0; i < cell.size(); i++) {
        cell_in_global_node_ids[i] = global_to_local_node_ids.at(cell[i]);
    }

    return cell_in_global_node_ids;
}
std::vector<int> Redistributor::getCellNodeOwners(const std::vector<int>& cell, const std::vector<int>& node_owners) {
    std::vector<int> owners(cell.size());
    for (size_t i = 0; i < cell.size(); i++) {
        owners[i] = node_owners.at(cell[i]);
    }
    return owners;
}
