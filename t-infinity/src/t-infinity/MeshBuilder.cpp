#include "MeshBuilder.h"
#include "Cell.h"
#include "parfait/Checkpoint.h"
#include <parfait/ToString.h>
#include <parfait/SyncPattern.h>
#include <parfait/Throw.h>
#include <parfait/VectorTools.h>
#include <t-infinity/MeshShuffle.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/Geometry.h>
#include <t-infinity/MeshQualityMetrics.h>
#include <parfait/VectorTools.h>

using namespace inf;
using namespace experimental;

void MeshBuilder::initializeConnectivity() {
    resetCountsForOffsets();
    rebuildNodeToCells();
    setUpModifiabilityForNodes();
    rebuildCellResidency();
}
void MeshBuilder::resetCountsForOffsets() {
    original_max_global_node_id = findLargestGlobalNodeId();
    original_max_global_cell_id = findLargestGlobalCellId();
    next_cell_gid = original_max_global_cell_id + 1;
    next_node_gid = original_max_global_node_id + 1;
}
MeshBuilder::MeshBuilder(MessagePasser mp)
    : mp(mp), mesh(std::make_shared<TinfMesh>(TinfMeshData(), mp.Rank())) {
    dimension = inf::maxCellDimensionality(mp, *mesh);
    initializeConnectivity();
}
MeshBuilder::MeshBuilder(MessagePasser mp, std::shared_ptr<MeshInterface>&& mesh_in)
    : mp(mp), mesh(std::make_shared<TinfMesh>(mesh_in)) {
    dimension = inf::maxCellDimensionality(mp, *mesh);
    mesh_in.reset();
    mesh_in = nullptr;
    initializeConnectivity();
}
MeshBuilder::MeshBuilder(MessagePasser mp, const MeshInterface& mesh_in)
    : mp(mp), mesh(std::make_shared<TinfMesh>(mesh_in)) {
    dimension = inf::maxCellDimensionality(mp, *mesh);
    initializeConnectivity();
}
void MeshBuilder::rebuildNodeToCells() {
    node_to_cells.clear();
    node_to_cells.resize(mesh->nodeCount());
    int node_count = mesh->nodeCount();

    for (auto& pair : mesh->mesh.cells) {
        auto type = pair.first;
        auto length = MeshInterface::cellTypeLength(type);
        auto& cells = mesh->mesh.cells.at(type);
        long count = cells.size() / length;
        for (int c = 0; c < count; c++) {
            CellEntry entry{type, c};
            for (int i = 0; i < length; i++) {
                int n = cells[length * c + i];
                if (n < node_count) node_to_cells[n].insert(entry);
            }
        }
    }
}
int MeshBuilder::findLastNotDeletedEntry(const std::set<int>& other_deleted, int last) {
    do {
        if (other_deleted.count(last) != 1) {
            return last;
        } else {
            last--;
        }
    } while (last >= 0);
    return -1;
}

void MeshBuilder::relabelNodeInCells(const CellEntry& cell_entry, int from, int to) {
    int length = MeshInterface::cellTypeLength(cell_entry.type);
    auto cell_nodes = &mesh->mesh.cells.at(cell_entry.type)[length * cell_entry.index];
    for (int i = 0; i < length; i++) {
        if (cell_nodes[i] == from) cell_nodes[i] = to;
    }
}
void MeshBuilder::throwIfDeletingMoreThanAdding() const {
    if (mp.NumberOfProcesses() > 1) {
        bool am_i_deleting_more_cells_than_adding = not unused_cell_gids.empty();
        am_i_deleting_more_cells_than_adding = mp.ParallelOr(am_i_deleting_more_cells_than_adding);
        PARFAIT_ASSERT(
            not am_i_deleting_more_cells_than_adding,
            "MeshBuilder can't delete more cells than it creates in parallel. These still exist:" +
                Parfait::to_string(unused_cell_gids));
        bool am_i_deleting_more_nodes_than_adding = not unused_node_gids.empty();
        am_i_deleting_more_nodes_than_adding = mp.ParallelOr(am_i_deleting_more_nodes_than_adding);
        PARFAIT_ASSERT(
            not am_i_deleting_more_nodes_than_adding,
            "MeshBuilder can't delete more nodes than it creates in parallel. These still exist:" +
                Parfait::to_string(unused_node_gids));
    }
}
void MeshBuilder::sync() {
    Tracer::begin("builder::sync");
    rebuildLocal();
    makeNewlyCreatedNodesHaveUniqueGids();  // does not fill holes
    makeNewlyCreatedCellsHaveUniqueGids();

    compactNodeGids();
    compactCellGids();

    resetCountsForOffsets();
    Tracer::end("builder::sync");
}
void MeshBuilder::makeNewlyCreatedNodesHaveUniqueGids() {
    long num_added_node_gids = new_node_gids.size();
    auto each_part_added_node_gids = mp.Gather(num_added_node_gids);
    long offset = 0;
    for (int r = 0; r < mp.Rank(); r++) {
        offset += each_part_added_node_gids[r];
    }
    for (auto& gid : mesh->mesh.global_node_id) {
        if (gid > original_max_global_node_id) {
            gid += offset;
        }
    }
}
void MeshBuilder::makeNewlyCreatedCellsHaveUniqueGids() {
    long num_added_cell_gids = new_cell_gids.size();
    auto each_part_added_cell_gids = mp.Gather(num_added_cell_gids);
    long offset = 0;
    for (int r = 0; r < mp.Rank(); r++) {
        offset += each_part_added_cell_gids[r];
    }
    for (auto& pair : mesh->mesh.global_cell_id) {
        for (auto& gid : pair.second)
            if (gid > original_max_global_cell_id) {
                gid += offset;
            }
    }
}
long MeshBuilder::claimNextCellGid() {
    if (unused_cell_gids.empty()) {
        new_cell_gids.insert(next_cell_gid);
        next_cell_gid++;
        return next_cell_gid - 1;
    } else {
        long next = *unused_cell_gids.begin();
        unused_cell_gids.erase(next);
        return next;
    }
}
long MeshBuilder::claimNextNodeGid() {
    if (unused_node_gids.empty()) {
        new_node_gids.insert(next_node_gid);
        next_node_gid++;
        return next_node_gid - 1;
    } else {
        long next = *unused_node_gids.begin();
        unused_node_gids.erase(next);
        return next;
    }
}
CellEntry MeshBuilder::addCell(MeshInterface::CellType type,
                               const std::vector<int>& cell_nodes,
                               int tag) {
    if (type == inf::MeshInterface::NODE) {
        int node_id = *cell_nodes.begin();
        is_node_modifiable[node_id] = false;
    }
    int length = MeshInterface::cellTypeLength(type);
    if (int(cell_nodes.size()) != length) {
        PARFAIT_THROW("Can not add " + std::to_string(cell_nodes.size()) +
                      " nodes as a cell of type " + MeshInterface::cellTypeString(type));
    }
    long gid = claimNextCellGid();
    if (unused_cell_type_indices.count(type) == 0) {
        unused_cell_type_indices[type] = {};
    }
    if (unused_cell_type_indices.at(type).empty()) {
        int new_cell_index = mesh->mesh.cells[type].size() / length;
        mesh->mesh.cells[type].resize(mesh->mesh.cells[type].size() + length);
        mesh->mesh.cell_owner[type].push_back(mp.Rank());
        mesh->mesh.cell_tags[type].push_back(tag);
        mesh->mesh.global_cell_id[type].push_back(gid);
        is_cell_modifiable[type].push_back(true);
        for (int i = 0; i < length; i++) {
            mesh->mesh.cells[type][length * new_cell_index + i] = cell_nodes[i];
            node_to_cells[cell_nodes[i]].insert({type, new_cell_index});
        }
        cell_ids_that_have_been_modified.insert({type, new_cell_index});
        return {type, new_cell_index};
    } else {
        int index_to_overwrite = *unused_cell_type_indices.at(type).begin();
        unused_cell_type_indices.at(type).erase(index_to_overwrite);
        mesh->mesh.cell_owner[type][index_to_overwrite] = mp.Rank();
        mesh->mesh.cell_tags[type][index_to_overwrite] = tag;
        mesh->mesh.global_cell_id[type][index_to_overwrite] = gid;
        is_cell_modifiable[type][index_to_overwrite] = true;
        for (int i = 0; i < length; i++) {
            mesh->mesh.cells[type][length * index_to_overwrite + i] = cell_nodes[i];
            node_to_cells[cell_nodes[i]].insert({type, index_to_overwrite});
        }
        return {type, index_to_overwrite};
    }
}

std::set<int> buildSetFromRagged(const std::vector<std::vector<int>>& ragged_array) {
    std::set<int> set;
    for (auto& f : ragged_array) {
        for (auto n : f) {
            set.insert(n);
        }
    }

    return set;
}

std::vector<CellTypeAndNodeIds> MeshBuilder::generateCellsToAdd(const MikeCavity& cavity,
                                                                int steiner_node_id) {
    std::vector<CellTypeAndNodeIds> cells_to_add;
    if (dimension == 3) {
        for (auto f : cavity.exposed_faces) {
            if (Parfait::VectorTools::isIn(f, steiner_node_id)) continue;
            if (f.size() == 3) {
                f.push_back(steiner_node_id);
                cells_to_add.push_back({MeshInterface::TETRA_4, f});
            } else if (f.size() == 4) {
                f.push_back(steiner_node_id);
                cells_to_add.push_back({MeshInterface::PYRA_5, f});
            } else if (f.size() == 2) {
                f.push_back(steiner_node_id);
                cells_to_add.push_back({MeshInterface::TRI_3, f});
            }
        }
    }
    if (dimension == 2) {
        for (auto f : cavity.exposed_faces) {
            if (Parfait::VectorTools::isIn(f, steiner_node_id)) continue;
            if (f.size() == 2) {
                f.push_back(steiner_node_id);
                cells_to_add.push_back({MeshInterface::TRI_3, f});
            } else if (f.size() == 1) {
                f.push_back(steiner_node_id);
                cells_to_add.push_back({MeshInterface::BAR_2, f});
            } else {
                PARFAIT_THROW("in 2D we can only handle BAR_2 exposed faces.");
            }
        }
    }
    return cells_to_add;
}

void MeshBuilder::replaceCavity(const MikeCavity& cavity, int steiner_node_id) {
    for (auto c : cavity.cells) {
        deleteCell(c.type, c.index);
    }
    auto cells_to_add = generateCellsToAdd(cavity, steiner_node_id);
    for (auto& c : cells_to_add) addCell(c.type, c.node_ids);
}

bool MeshBuilder::replaceCavityTryQuads(const MikeCavity& cavity,
                                        int steiner_node_id,
                                        double cost_threshold) {
    for (auto c : cavity.cells) {
        deleteCell(c.type, c.index);
    }
    bool all_triangles = true;
    for (auto c : cavity.cells) {
        if (c.type != inf::MeshInterface::TRI_3) all_triangles = false;
    }
    if (all_triangles and 4 == cavity.exposed_faces.size()) {
        std::vector<int> quad;
        std::vector<Parfait::Point<double>> points;
        auto faces = windFacesHeadToTail(cavity.exposed_faces);
        for (auto& face : faces) {
            int left_node_id = face.front();
            quad.push_back(left_node_id);
            points.push_back(mesh->node(left_node_id));
        }
        double cost = MeshQuality::cellHilbertCost(MeshInterface::QUAD_4, points);
        if (cost < cost_threshold) {
            //            printf("Quad would be %lf cost\n",cost);
            //            printf("adding quad: %i %i %i %i\n",quad[0],quad[1],quad[2],quad[3]);
            addCell(MeshInterface::QUAD_4, quad);
            return true;
        }
    }
    replaceCavity(cavity, steiner_node_id);
    return false;
}

void MeshBuilder::steinerCavity(const std::vector<std::vector<int>>& faces_pointing_into_cavity) {
    auto nodes_in_cavity_faces = buildSetFromRagged(faces_pointing_into_cavity);
    Parfait::Point<double> p = {0, 0, 0};
    for (auto n : nodes_in_cavity_faces) {
        p += mesh->mesh.points[n];
    }
    p /= double(nodes_in_cavity_faces.size());

    int steiner_node_id = addNode(p);

    for (auto f : faces_pointing_into_cavity) {
        if (f.size() == 3) {
            f.push_back(steiner_node_id);
            addCell(MeshInterface::TETRA_4, f);
        } else if (f.size() == 4) {
            f.push_back(steiner_node_id);
            addCell(MeshInterface::PYRA_5, f);
        } else if (f.size() == 2) {
            f.push_back(steiner_node_id);
            addCell(MeshInterface::TRI_3, f);
        }
    }
}

void MeshBuilder::removeLazyDeletedNodes() {
    int copy_from = int(mesh->mesh.points.size()) - 1;
    for (auto to_be_deleted : unused_local_node_ids) {
        copy_from = findLastNotDeletedEntry(unused_local_node_ids, copy_from);
        if (copy_from >= to_be_deleted) {
            mesh->mesh.points[to_be_deleted] = mesh->mesh.points[copy_from];
            mesh->mesh.global_node_id[to_be_deleted] = mesh->mesh.global_node_id[copy_from];
            mesh->mesh.node_owner[to_be_deleted] = mesh->mesh.node_owner[copy_from];
            for (auto& neighbor : node_to_cells[copy_from]) {
                relabelNodeInCells(neighbor, copy_from, to_be_deleted);
            }
            node_to_cells[to_be_deleted] = node_to_cells[copy_from];
            is_node_modifiable[to_be_deleted] = is_node_modifiable[copy_from];
            node_geom_association[to_be_deleted] = node_geom_association[copy_from];
            node_geom_association.erase(copy_from);
            copy_from--;
        }

        mesh->mesh.node_owner.pop_back();
        mesh->mesh.global_node_id.pop_back();
        mesh->mesh.points.pop_back();
        is_node_modifiable.pop_back();
    }
    unused_local_node_ids.clear();
}
void MeshBuilder::removeLazyDeletedCells() {
    for (auto pair : unused_cell_type_indices) {
        auto type = pair.first;
        auto to_be_deleted = pair.second;
        int length = MeshInterface::cellTypeLength(type);
        auto& cells = mesh->mesh.cells[type];
        auto& owners = mesh->mesh.cell_owner[type];
        auto& tags = mesh->mesh.cell_tags[type];
        auto& gids = mesh->mesh.global_cell_id[type];
        auto& modifiable = is_cell_modifiable.at(type);

        int copy_from = int(mesh->mesh.cells.at(type).size() / length) - 1;

        for (int id : to_be_deleted) {
            copy_from = findLastNotDeletedEntry(unused_cell_type_indices.at(type), copy_from);

            if (copy_from >= id) {
                for (int i = 0; i < length; i++) {
                    int node = cells[copy_from * length + i];
                    if (node < 0) continue;
                    auto old_entry = CellEntry{type, copy_from};
                    auto new_entry = CellEntry{type, id};
                    auto& neighbors = node_to_cells.at(cells[copy_from * length + i]);
                    neighbors.erase(old_entry);
                    neighbors.insert(new_entry);
                    cells[id * length + i] = cells[copy_from * length + i];
                }
                owners[id] = owners[copy_from];
                tags[id] = tags[copy_from];
                gids[id] = gids[copy_from];
                modifiable[id] = modifiable[copy_from];
                copy_from--;
            }

            cells.resize(cells.size() - length);
            owners.pop_back();
            tags.pop_back();
            gids.pop_back();
            modifiable.pop_back();
        }
        if (mesh->mesh.cells[type].empty()) {
            mesh->mesh.cells.erase(type);
            mesh->mesh.cell_owner.erase(type);
            mesh->mesh.cell_tags.erase(type);
            mesh->mesh.global_cell_id.erase(type);
            is_cell_modifiable.erase(type);
        }
    }
    unused_cell_type_indices.clear();
}
long MeshBuilder::findLargestGlobalCellId() const {
    long biggest = -1;
    for (auto& pair : mesh->mesh.global_cell_id) {
        auto& gids = pair.second;
        for (auto g : gids) {
            biggest = std::max(biggest, g);
        }
    }
    return mp.ParallelMax(biggest);
}
long MeshBuilder::findLargestGlobalNodeId() const {
    long biggest = -1;
    for (auto g : mesh->mesh.global_node_id) {
        biggest = std::max(biggest, g);
    }
    return mp.ParallelMax(biggest);
}
void MeshBuilder::deleteNode(int node_id) {
    if (not isNodeModifiable(node_id)) {
        PARFAIT_THROW("Node is not deletable.  It has nonlocal residency");
    }
    unused_local_node_ids.insert(node_id);
    int owner = mesh->mesh.node_owner.at(node_id);
    if (owner == mp.Rank()) {
        auto gid = mesh->mesh.global_node_id.at(node_id);
        unused_node_gids.insert(gid);
    }
    node_to_cells[0].clear();
}
void MeshBuilder::deleteCell(MeshInterface::CellType type, int type_index) {
    if (not isCellModifiable({type, type_index})) {
        PARFAIT_THROW("Cell is not deletable.  It has nonlocal residency");
    }
    cell_ids_that_have_been_modified.insert({type, type_index});
    if (unused_cell_type_indices[type].count(type_index) == 1) return;
    unused_cell_type_indices[type].insert(type_index);

    new_cell_gids.erase(mesh->mesh.global_cell_id.at(type).at(type_index));

    int owner = mesh->mesh.cell_owner.at(type).at(type_index);
    if (owner == mp.Rank()) {
        auto gid = mesh->mesh.global_cell_id.at(type).at(type_index);
        unused_cell_gids.insert(gid);
    } else {
        PARFAIT_THROW("Deleting non owned cell " + MeshInterface::cellTypeString(type) + " " +
                      std::to_string(type_index));
    }
    int length = MeshInterface::cellTypeLength(type);
    auto cell = &mesh->mesh.cells.at(type)[length * type_index];
    for (int i = 0; i < length; i++) {
        node_to_cells[cell[i]].erase({type, type_index});
        cell[i] = -1;
    }
}
int MeshBuilder::addNode(Parfait::Point<double> p) {
    long gid = claimNextNodeGid();
    if (unused_local_node_ids.empty()) {
        mesh->mesh.points.push_back(p);
        mesh->mesh.global_node_id.push_back(gid);
        mesh->mesh.node_owner.push_back(mp.Rank());
        node_to_cells.resize(node_to_cells.size() + 1);
        is_node_modifiable.push_back(true);
        return int(mesh->mesh.points.size()) - 1;
    } else {
        int index_to_overwrite = *unused_local_node_ids.begin();
        unused_local_node_ids.erase(index_to_overwrite);
        mesh->mesh.points[index_to_overwrite] = p;
        mesh->mesh.global_node_id[index_to_overwrite] = gid;
        mesh->mesh.node_owner[index_to_overwrite] = mp.Rank();
        is_node_modifiable[index_to_overwrite] = true;
        node_to_cells[index_to_overwrite] = std::set<CellEntry>();
        return index_to_overwrite;
    }
}
void MeshBuilder::rebuildLocal() {
    removeLazyDeletedNodes();
    removeLazyDeletedCells();
    cell_ids_that_have_been_modified.clear();
    mesh->rebuild();
    setUpModifiabilityForNodes();
}
void MeshBuilder::printNodeToCells() {
    printf("%d nodes\n", mesh->nodeCount());
    for (int n = 0; n < mesh->nodeCount(); n++) {
        printf("%d connected to cells: ", n);
        for (auto& c : node_to_cells[n]) {
            printf("%s %d, ", MeshInterface::cellTypeString(c.type).c_str(), c.index);
        }
        printf("\n");
    }
}
void MeshBuilder::printCells() {
    for (auto& pair : mesh->mesh.cells) {
        auto length = MeshInterface::cellTypeLength(pair.first);
        int count = int(pair.second.size() / length);
        printf("%d cells of type %s\n", count, MeshInterface::cellTypeString(pair.first).c_str());
        for (int c = 0; c < count; c++) {
            printf("%d, %ld: ", c, mesh->mesh.global_cell_id.at(pair.first)[c]);
            for (int i = 0; i < length; i++) {
                printf("%d ", pair.second[length * c + i]);
            }
            printf("\n");
        }
    }
}
void MeshBuilder::printNodes() {
    printf("%d nodes\n", mesh->nodeCount());
    for (int n = 0; n < mesh->nodeCount(); n++) {
        Parfait::Point<double> p = mesh->node(n);
        printf("%d %ld, %s\n", n, mesh->globalNodeId(n), p.to_string().c_str());
    }
}
void MeshBuilder::compactNodeGids() {
    std::vector<long> have, need;
    std::vector<int> need_from;
    for (int n = 0; n < mesh->nodeCount(); n++) {
        if (mesh->nodeOwner(n) == mp.Rank()) {
            have.push_back(mesh->globalNodeId(n));
        } else {
            need.push_back(mesh->globalNodeId(n));
            need_from.push_back(mesh->nodeOwner(n));
        }
    }
    auto old_g2l = GlobalToLocal::buildNode(*mesh);
    auto sync_pattern = Parfait::SyncPattern(mp, have, need, need_from);

    long num_owned = have.size();
    auto all_owned_counts = mp.Gather(num_owned);

    long offset = 0;
    for (int r = 0; r < mp.Rank(); r++) {
        offset += all_owned_counts[r];
    }

    std::vector<long> my_new_node_gids(mesh->nodeCount());
    for (int n = 0; n < mesh->nodeCount(); n++) {
        if (mesh->nodeOwner(n) == mp.Rank()) {
            my_new_node_gids[n] = offset++;
        }
    }

    Parfait::syncVector(mp, my_new_node_gids, old_g2l, sync_pattern);

    for (int n = 0; n < mesh->nodeCount(); n++) {
        mesh->mesh.global_node_id[n] = my_new_node_gids[n];
    }
}
void MeshBuilder::compactCellGids() {
    std::vector<long> have, need;
    std::vector<int> need_from;
    for (int n = 0; n < mesh->cellCount(); n++) {
        if (mesh->cellOwner(n) == mp.Rank()) {
            have.push_back(mesh->globalCellId(n));
        } else {
            need.push_back(mesh->globalCellId(n));
            need_from.push_back(mesh->cellOwner(n));
        }
    }
    auto old_g2l = GlobalToLocal::buildCell(*mesh);
    auto sync_pattern = Parfait::SyncPattern(mp, have, need, need_from);

    long num_owned = have.size();
    auto all_owned_counts = mp.Gather(num_owned);

    long offset = 0;
    for (int r = 0; r < mp.Rank(); r++) {
        offset += all_owned_counts[r];
    }

    std::vector<long> my_new_cell_gids(mesh->cellCount());
    for (int n = 0; n < mesh->cellCount(); n++) {
        if (mesh->cellOwner(n) == mp.Rank()) {
            my_new_cell_gids[n] = offset++;
        }
    }

    Parfait::syncVector(mp, my_new_cell_gids, old_g2l, sync_pattern);

    for (int c = 0; c < mesh->cellCount(); c++) {
        auto entry = mesh->cellIdToTypeAndLocalId(c);
        mesh->mesh.global_cell_id.at(entry.first)[entry.second] = my_new_cell_gids[c];
    }
}
bool MeshBuilder::isNodeModifiable(int node_id) const { return is_node_modifiable[node_id]; }
bool MeshBuilder::isCellModifiable(CellEntry cell) const {
    return is_cell_modifiable.at(cell.type)[cell.index];
}
void MeshBuilder::setUpModifiabilityForNodes() {
    is_node_modifiable.resize(mesh->nodeCount());
    markNodesNearPartitionBoundaries();
    markNodesThatAreCriticalOnGeometry();
}
void MeshBuilder::markNodesNearPartitionBoundaries() {
    std::vector<long> have, need;
    std::vector<int> need_from;
    for (int n = 0; n < mesh->nodeCount(); n++) {
        auto gid = mesh->globalNodeId(n);
        auto owner = mesh->nodeOwner(n);
        if (owner == mesh->partitionId()) {
            have.push_back(gid);
            is_node_modifiable[n] = true;
        } else {
            need.push_back(gid);
            need_from.push_back(owner);
            is_node_modifiable[n] = false;
        }
    }
    auto g2l = GlobalToLocal::buildNode(*mesh);
    auto sp = Parfait::SyncPattern(mp, have, need, need_from);
    for (auto& pair : sp.send_to) {
        for (auto gid : pair.second) {
            auto local = g2l.at(gid);
            is_node_modifiable[local] = false;
        }
    }
}
void MeshBuilder::rebuildCellResidency() {
    std::vector<long> have, need;
    std::vector<int> need_from;
    for (auto& pair : mesh->mesh.cell_tags) {
        is_cell_modifiable[pair.first].resize(pair.second.size());
    }

    for (int c = 0; c < mesh->cellCount(); c++) {
        auto gid = mesh->globalCellId(c);
        auto owner = mesh->cellOwner(c);
        auto entry = mesh->cellIdToTypeAndLocalId(c);
        if (owner == mesh->partitionId()) {
            have.push_back(gid);
            is_cell_modifiable[entry.first][entry.second] = true;
        } else {
            need.push_back(gid);
            need_from.push_back(owner);
            is_cell_modifiable[entry.first][entry.second] = false;
        }
    }

    auto g2l = GlobalToLocal::buildCell(*mesh);
    auto sp = Parfait::SyncPattern(mp, have, need, need_from);
    for (auto& pair : sp.send_to) {
        for (auto gid : pair.second) {
            auto local = g2l.at(gid);
            auto entry = mesh->cellIdToTypeAndLocalId(local);
            is_cell_modifiable[entry.first][entry.second] = false;
        }
    }
}
CellView MeshBuilder::cell(MeshInterface::CellType type, int type_index) const {
    auto length = MeshInterface::cellTypeLength(type);
    int* ptr = &mesh->mesh.cells[type][length * type_index];
    return {ptr, size_t(length)};
}

Cavity MeshBuilder::findCavity(const std::set<int>& nodes) {
    Cavity cavity;

    for (int n : nodes) {
        auto& neighbors = node_to_cells[n];
        for (auto& c : neighbors) {
            cavity.cells.insert(c);
            auto cell = this->cell(c.type, c.index);
            for (int i = 0; i < cell.size(); i++) {
                auto a = cell[(i + 1) % cell.size()];
                auto b = cell[(i + 2) % cell.size()];
                if (nodes.count(a) == 1 or nodes.count(b) == 1) continue;
                cavity.exposed_faces.push_back({a, b});
            }
        }
    }
    return cavity;
}
MeshBuilder::Status MeshBuilder::edgeCollapse(int a, int b) {
    if (not isNodeModifiable(a)) return FAILURE;
    if (not isNodeModifiable(b)) return FAILURE;
    auto cavity = findCavity({a, b});
    for (auto c : cavity.cells) {
        if (not isCellModifiable(c)) return FAILURE;
    }
    deleteNode(a);
    deleteNode(b);
    for (auto c : cavity.cells) {
        deleteCell(c.type, c.index);
    }
    steinerCavity(cavity.exposed_faces);
    return SUCCESS;
}

std::pair<CellEntry, CellEntry> MeshBuilder::edgeSurfaceNeighbors(int a, int b) {
    auto& a_neighbors = node_to_cells[a];
    auto& b_neighbors = node_to_cells[b];
    CellEntry c1{inf::MeshInterface::CellType::NODE, -1};
    CellEntry c2{inf::MeshInterface::CellType::NODE, -1};
    bool set_first = false;
    bool set_second = false;
    for (auto c : a_neighbors) {
        if (b_neighbors.count(c) == 1) {
            if (not set_first) {
                c1 = c;
                set_first = true;
            } else if (not set_second) {
                c2 = c;
                set_second = true;
            } else {
                PARFAIT_THROW("Broken assomption ")
            }
        }
    }
    PARFAIT_ASSERT(set_first and set_second,
                   "Could not find both cells adjacent to a surface edge");
    return {c1, c2};
}

MeshBuilder::Status MeshBuilder::surfaceEdgeSplit(int a, int b) {
    if (not isNodeModifiable(a)) return FAILURE;
    if (not isNodeModifiable(b)) return FAILURE;

    auto neighboring_cells = edgeSurfaceNeighbors(a, b);
    if (not isCellModifiable(neighboring_cells.first)) return FAILURE;
    if (not isCellModifiable(neighboring_cells.second)) return FAILURE;

    Parfait::Point<double> pa = mesh->mesh.points[a];
    Parfait::Point<double> pb = mesh->mesh.points[b];
    Parfait::Point<double> p = 0.5 * (pa + pb);
    auto pn = addNode(p);
    bool s = splitEdgeInSurfaceCell(a, b, pn, neighboring_cells.first);
    s |= splitEdgeInSurfaceCell(a, b, pn, neighboring_cells.second);

    return Status(s);
}
bool MeshBuilder::splitEdgeInSurfaceCell(int a, int b, int pn, CellEntry cell) {
    auto cell_nodes = this->cell(cell.type, cell.index);

    for (int e = 0; e < cell_nodes.size(); e++) {
        int ea = cell_nodes[e];
        int eb = cell_nodes[(e + 1) % cell_nodes.size()];
        if ((ea == a and eb == b) or (ea == b and eb == a)) {
            // skip the edge that we're splitting
            continue;
        }
        if (ea == a or ea == b) {
            addCell(inf::MeshInterface::TRI_3, {ea, eb, pn});
        } else if (eb == a or eb == b) {
            addCell(inf::MeshInterface::TRI_3, {pn, ea, eb});
        } else {
            addCell(inf::MeshInterface::TRI_3, {ea, eb, pn});
        }
    }

    deleteCell(cell.type, cell.index);
    return Status::SUCCESS;
}
CellEntry MeshBuilder::findHostCellId(const Parfait::Point<double>& p, CellEntry search_cell) {
    std::set<CellEntry> already_searched_cells;
    while (not isPointInCell(p, search_cell)) {
        already_searched_cells.insert(search_cell);
        search_cell = findNextWalkingCell(p, search_cell);
        if (search_cell.isNull()) {
            PARFAIT_THROW("Could not walk to find host cell");
        }
        if (already_searched_cells.count(search_cell)) {
            PARFAIT_WARNING("Walking in a circle");
            return search_cell;
        }
    }
    return search_cell;
}

CellEntry MeshBuilder::findHostCellIdReallySlow(const Parfait::Point<double>& p) {
    int check_count = 0;
    for (auto& pair : mesh->mesh.cells) {
        auto type = pair.first;
        auto& cells = pair.second;
        if (inf::MeshInterface::cellTypeDimensionality(type) == dimension) {
            int cell_length = inf::MeshInterface::cellTypeLength(type);
            int num_cells_in_type = cells.size() / cell_length;
            for (int index = 0; index < num_cells_in_type; index++) {
                inf::Cell cell(*mesh, type, index, dimension);
                check_count++;
                if (cell.contains(p)) {
                    return {type, index};
                }
            }
        }
    }
    printf("findCell, dimension %d\n", dimension);
    PARFAIT_THROW("Literally no one has this point.  We checked: " + std::to_string(check_count));
}

bool MeshBuilder::isPointInCell(const Parfait::Point<double>& p, CellEntry search_cell) {
    inf::Cell cell(*mesh, search_cell.type, search_cell.index);
    return cell.contains(p);
}
CellEntry MeshBuilder::findNextWalkingCell(const Parfait::Point<double>& p, CellEntry search_cell) {
    inf::Cell cell(*mesh, search_cell.type, search_cell.index);
    auto next_cell = CellEntry::null();
    double max_distance = std::numeric_limits<double>::max();
    for (auto n : cell.nodes()) {
        for (auto c : node_to_cells[n]) {
            inf::Cell neighbor(*mesh, c.type, c.index);
            auto centroid = neighbor.averageCenter();
            double dist = (centroid - p).magnitude();
            if (dist < max_distance) {
                max_distance = dist;
                next_cell = c;
            }
        }
    }
    return next_cell;
}
inf::MeshInterface::CellType findAValidVolumeCellType(const inf::TinfMesh& mesh, int dimension) {
    for (auto& pair : mesh.mesh.cells) {
        auto type = pair.first;
        if (inf::MeshInterface::cellTypeDimensionality(type) == dimension) {
            return type;
        }
    }
    PARFAIT_THROW("Cound not find any " + std::to_string(dimension) + "D cells");
}
void MeshBuilder::insertPoint(Parfait::Point<double> p) {
    //    auto seed_type = findAValidVolumeCellType(*mesh, dimension);
    //    auto cell = findHostCellId(p, {seed_type, 0});
    auto cell = findHostCellIdReallySlow(p);
    MikeCavity cavity(dimension);
    cavity.addCell(*mesh, cell);

    auto steiner_node_id = addNode(p);
    replaceCavity(cavity, steiner_node_id);
}

double MeshBuilder::calcWorstCellCostAtNode(int node_id) const {
    double worst_cost = 0.0;
    for (auto& neighbor : node_to_cells[node_id]) {
        inf::Cell cell(*mesh, neighbor.type, neighbor.index);
        auto cost = inf::MeshQuality::cellHilbertCost(cell.type(), cell.points());
        worst_cost = std::max(cost, worst_cost);
    }
    return worst_cost;
}

double MeshBuilder::smooth(
    int node_id,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> edge_length_function,
    inf::geometry::DatabaseInterface* db,
    int max_steps) {
    if (not isNodeModifiable(node_id))
        return 0.0;  // do not smooth nodes resident on other ranks, or nodes on critical geom
                     // points

    if (isNodeOnGeometryEdge(node_id)) {
        PARFAIT_ASSERT(db != nullptr,
                       "Can't move node on geometry if you don't tell us the geometry");
        smoothNodeOnGeomEdge(node_id, edge_length_function, db);
        return 0.0;
    }

    std::set<int> neighbor_nodes;
    for (auto c : node_to_cells[node_id]) {
        auto& cells = mesh->mesh.cells[c.type];
        auto length = inf::MeshInterface::cellTypeLength(c.type);
        auto* cell_nodes = &cells[length * c.index];
        for (int i = 0; i < length; i++) {
            neighbor_nodes.insert(cell_nodes[i]);
        }
    }
    neighbor_nodes.erase(node_id);

    //    double max_length_threshold = 2.0;
    //    double min_length_threshold = 0.5;
    //    for (int neighbor : neighbor_nodes) {
    //        auto p = mesh->mesh.points[node_id];
    //        auto q = mesh->mesh.points[neighbor];
    //        double d = edge_length_function(p, q);
    //        if (d < min_length_threshold) return 0.0;
    //        if (d > max_length_threshold) return 0.0;
    //    }
    //
    auto original_node_location = mesh->mesh.points[node_id];
    double worst_neighbor_cost = calcWorstCellCostAtNode(node_id);
    //    printf("starting worst cell cost %e\n", worst_neighbor_cost);

    for (int step = 0; step < max_steps; step++) {
        auto p = mesh->mesh.points[node_id];
        Parfait::Point<double> delta_p = {0, 0, 0};
        double alpha = 0.2;  // fixed in Bossen & Heckbert
                             // people.eecs.berkeley.edu/~jrs/meshpapers/BossenHeckbert.pdf
        for (int neighbor : neighbor_nodes) {
            auto q = mesh->mesh.points[neighbor];
            auto v = q - p;
            double d = edge_length_function(p, q);
            if (d > std::numeric_limits<double>::epsilon()) {
                v /= d;
                double d4 = d * d * d * d;
                double f = (1.0 - d4) * std::exp(-d4);
                //            d = (1 - pow(d, 4)) * exp(-pow(d, 4));
                //            delta_p += d * v;
                delta_p += f * v;
                //                printf("  edge: %s d %e, f(d) %e\n", v.to_string().c_str(), d, f);
            }
        }
        if (delta_p.magnitude() < 1e-13) {
            break;
        }
        //        alpha = 1.0 / (double)neighbor_nodes.size();
        mesh->mesh.points[node_id] = p - alpha * delta_p;
        worst_neighbor_cost = calcWorstCellCostAtNode(node_id);
        //        PARFAIT_ASSERT(worst_neighbor_cost <= 1.0,"tangled the mesh");
        if (worst_neighbor_cost > 1.0) {
            mesh->mesh.points[node_id] = original_node_location;
        }
        //        printf("step %d dx %e worst_cell_cost %1.2lf\n", step, delta_p.magnitude(),
        //        worst_neighbor_cost);
    }
    return (original_node_location - mesh->mesh.points[node_id]).magnitude();
}
bool MeshBuilder::isNodeOnGeometryEdge(int node_id) const {
    if (node_geom_association.count(node_id) == 0) return false;
    for (auto& association : node_geom_association.at(node_id)) {
        if (association.type == inf::geometry::GeomAssociation::EDGE) {
            return true;
        }
    }
    return false;
}
bool MeshBuilder::isNodeOnBoundary(int node_id) {
    std::set<inf::MeshInterface::CellType> boundary_types;
    boundary_types.insert(inf::MeshInterface::CellType::NODE);
    boundary_types.insert(inf::MeshInterface::CellType::BAR_2);
    if (dimension == 3) {
        boundary_types.insert(inf::MeshInterface::CellType::TRI_3);
        boundary_types.insert(inf::MeshInterface::CellType::QUAD_4);
    }

    for (auto neighbor : node_to_cells[node_id]) {
        if (boundary_types.count(neighbor.type)) {
            return true;
        }
    }
    return false;
}
void MeshBuilder::growCavityAroundPoint(
    int node_id,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
    MikeCavity& cavity,
    bool include_boundaries) {
    auto is_this_actually_a_face = [=](const std::vector<int>& f) {
        if (int(f.size()) < dimension) return false;
        return true;
    };

    auto site_location = Parfait::Point<double>(mesh->node(node_id));

    auto is_this_face_too_close = [=](const std::vector<int>& f) {
        for (auto n : f) {
            auto p = mesh->mesh.points[n];
            auto length = calc_edge_length(site_location, p);
            if (length > 1.0) return false;
        }
        return true;
    };

    auto face_would_make_a_bad_element = [=](const std::vector<int>& f) {
        if (1 == std::count(f.begin(), f.end(), node_id)) return false;
        if (dimension == 2) {
            Parfait::Point<double> a = mesh->node(f[0]);
            Parfait::Point<double> b = mesh->node(f[1]);
            auto c = site_location;
            auto area_vector = (b - a).cross(c - a);
            double area = area_vector.dot({0, 0, 1});
            return area <= 0;
        } else {
            throw std::logic_error("Cavity expansion: Not implemented for 3D");
        }
    };

    auto face_is_bad = [=](const std::vector<int>& f) {
        return is_this_face_too_close(f) or face_would_make_a_bad_element(f);
    };

    std::set<CellEntry> cells_to_add;
    int cells_added_so_far = 0;
    do {
        cells_to_add.clear();
        for (auto& f : cavity.exposed_faces) {
            if (not is_this_actually_a_face(f)) continue;  // I don't remember why we need this?

            if (face_is_bad(f)) {
                auto neighbor_cells = findNeighborsOfFace(f);
                for (auto& neighbor : neighbor_cells) {
                    if (not cavity.cells.count(neighbor)) {
                        if (not isBoundaryType(neighbor.type) or include_boundaries) {
                            cells_to_add.insert(neighbor);
                        }
                    }
                }
            }
        }
        for (auto c : cells_to_add) {
            cavity.addCell(*mesh, c);
            cavity.visualizePoints(
                *mesh, "cavity-points-after-expanding-" + std::to_string(cells_added_so_far++));
        }
    } while (not cells_to_add.empty());
}

bool MeshBuilder::doesCellContainNodes(const std::vector<int>& cell_nodes,
                                       const std::vector<int>& face_nodes) {
    for (auto n : face_nodes) {
        auto count = std::count(cell_nodes.begin(), cell_nodes.end(), n);
        if (count == 0) return false;
    }
    return true;
}

std::vector<CellEntry> MeshBuilder::findNeighborsOfFace(const std::vector<int>& face_nodes) {
    std::set<CellEntry> neighboring_cells;
    for (auto n : face_nodes) {
        for (auto neighbor : node_to_cells[n]) {
            inf::Cell cell(*mesh, neighbor.type, neighbor.index);
            if (doesCellContainNodes(cell.nodes(), face_nodes)) {
                neighboring_cells.insert(neighbor);
            }
        }
    }

    return std::vector<CellEntry>(neighboring_cells.begin(), neighboring_cells.end());
}

void MeshBuilder::markNodesThatAreCriticalOnGeometry() {
    for (auto& pair : node_geom_association) {
        int node_id = pair.first;
        for (const auto& association : pair.second) {
            if (association.type == inf::geometry::GeomAssociation::NODE)
                is_node_modifiable[node_id] = false;
        }
    }
}
void MeshBuilder::smoothNodeOnGeomEdge(
    int node_id,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> edge_length_function,
    geometry::DatabaseInterface* db) {
    // If there are multiple geom associations then this node should be a critical node
    PARFAIT_ASSERT(node_geom_association.at(node_id).size(),
                   "Can't move nodes that are on multiple geometry associations")
    auto& edge_association = node_geom_association.at(node_id)[0];

    std::set<int> neighbor_nodes;
    for (auto& neighboring_cell : node_to_cells[node_id]) {
        if (neighboring_cell.type != inf::MeshInterface::BAR_2) continue;
        int node_a = mesh->mesh.cells.at(neighboring_cell.type)[neighboring_cell.index * 2 + 0];
        int node_b = mesh->mesh.cells.at(neighboring_cell.type)[neighboring_cell.index * 2 + 1];
        int neighbor_node = node_a == node_id ? node_b : node_a;
        neighbor_nodes.insert(neighbor_node);
    }
    // There should only be two neighbors for a node that is on a CAD edge.
    PARFAIT_ASSERT(neighbor_nodes.size() == 2, "Found too many neighboring nodes on bars");
    auto neighbors = std::vector<int>(neighbor_nodes.begin(), neighbor_nodes.end());

    auto edge = db->getEdge(edge_association.index);
    int neighbor_a = neighbors[0];
    int neighbor_b = neighbors[1];

    int num_samples = 100;
    double dt = 1.0 / (num_samples - 1);
    Parfait::Point<double> a = mesh->node(neighbor_a);
    Parfait::Point<double> b = mesh->node(neighbor_b);
    double best_ratio = std::numeric_limits<double>::max();
    Parfait::Point<double> best_point;
    for (int i = 0; i < num_samples; i++) {
        double t = i * dt;
        auto p = edge->project((1.0 - t) * a + t * b);
        double a_length = edge_length_function(a, p);
        double b_length = edge_length_function(b, p);
        double r1 = b_length / a_length;
        double r2 = a_length / b_length;
        double r = std::max(r1, r2);
        if (r < best_ratio) {
            best_ratio = r;
            best_point = p;
        }
    }
    mesh->mesh.points[node_id] = best_point;
}
inf::geometry::GeomAssociation MeshBuilder::calcEdgeGeomAssociation(int a, int b) const {
    using T = inf::geometry::GeomAssociation::Type;
    PARFAIT_ASSERT_KEY(node_geom_association, a)
    PARFAIT_ASSERT_KEY(node_geom_association, b)
    for (auto& association_a : node_geom_association.at(a)) {
        if (association_a.type != T::EDGE) continue;
        for (auto& association_b : node_geom_association.at(b)) {
            if (association_b.type == T::EDGE and association_b.index == association_a.index) {
                geometry::GeomAssociation edge_midpoint_association = association_a;
                return edge_midpoint_association;
            }
        }
    }
    PARFAIT_THROW("Could not find shared edge association between nodes " + std::to_string(a) +
                  " and " + std::to_string(b));
}

MeshBuilder::MeshBuilder(const MeshBuilder& rhs) : mp(rhs.mp) {
    mesh = std::make_shared<TinfMesh>(rhs.mesh->mesh, rhs.mesh->partitionId());

    dimension = rhs.dimension;
    is_node_modifiable = rhs.is_node_modifiable;
    is_cell_modifiable = rhs.is_cell_modifiable;
    original_max_global_node_id = rhs.original_max_global_node_id;
    original_max_global_cell_id = rhs.original_max_global_cell_id;
    next_cell_gid = rhs.next_cell_gid;
    next_node_gid = rhs.next_node_gid;
    unused_cell_type_indices = rhs.unused_cell_type_indices;
    node_to_cells = rhs.node_to_cells;
    unused_local_node_ids = rhs.unused_local_node_ids;
    new_node_gids = rhs.new_node_gids;
    new_cell_gids = rhs.new_cell_gids;
    unused_cell_gids = rhs.unused_cell_gids;
    unused_node_gids = rhs.unused_node_gids;
    node_geom_association = rhs.node_geom_association;
}
std::vector<std::vector<int>> MeshBuilder::windFacesHeadToTail(
    std::vector<std::vector<int>> unwound_faces) {
    std::vector<std::vector<int>> wound_faces;

    int num_faces = unwound_faces.size();
    wound_faces.push_back(unwound_faces[0]);
    unwound_faces.erase(unwound_faces.begin());
    for (int i = 1; i < num_faces; i++) {
        int tail_node = wound_faces.back().back();
        for (int f = 0; f < int(unwound_faces.size()); f++) {
            if (unwound_faces[f][0] == tail_node) {
                wound_faces.push_back(unwound_faces[f]);
                unwound_faces.erase(unwound_faces.begin() + f);
                break;
            }
        }
    }
    return wound_faces;
}
bool MeshBuilder::isCellNull(CellEntry cell_entry) const {
    auto length = inf::MeshInterface::cellTypeLength(cell_entry.type);
    if (mesh->mesh.cells.count(cell_entry.type) == 0) {
        return true;
    }
    if (mesh->mesh.cells.at(cell_entry.type)[length * cell_entry.index] < 0) return true;
    return false;
}
