#include "MeshHelpers.h"

namespace inf {

Parfait::SyncPattern buildCellSyncPattern(MessagePasser mp, const MeshInterface& mesh) {
    std::vector<long> have, need;
    std::vector<int> need_from;
    for (int local = 0; local < mesh.cellCount(); local++) {
        long global = mesh.globalCellId(local);
        if (mesh.cellOwner(local) == mp.Rank()) {
            have.push_back(global);
        } else {
            need.push_back(global);
            need_from.push_back(mesh.cellOwner(local));
        }
    }
    return {mp, have, need, need_from};
}

Parfait::SyncPattern buildNodeSyncPattern(MessagePasser mp, const MeshInterface& mesh) {
    std::vector<long> have, need;
    std::vector<int> need_from;
    for (int local = 0; local < mesh.nodeCount(); local++) {
        long global = mesh.globalNodeId(local);
        if (mesh.nodeOwner(local) == mp.Rank()) {
            have.push_back(global);
        } else {
            need.push_back(global);
            need_from.push_back(mesh.nodeOwner(local));
        }
    }
    return {mp, have, need, need_from};
}
Parfait::SyncPattern buildNodeSyncPattern(MessagePasser mp,
                                          const MeshInterface& mesh,
                                          const std::set<int>& subset_of_non_owned_nodes_to_sync) {
    std::vector<long> have, need;
    std::vector<int> need_from;
    for (int local = 0; local < mesh.nodeCount(); local++) {
        if (mesh.nodeOwner(local) == mp.Rank()) have.push_back(mesh.globalNodeId(local));
    }
    for (int local : subset_of_non_owned_nodes_to_sync) {
        if (mesh.nodeOwner(local) != mp.Rank()) {
            need.push_back(mesh.globalNodeId(local));
            need_from.push_back(mesh.nodeOwner(local));
        }
    }
    return {mp, have, need, need_from};
}
long globalNodeCount(MessagePasser mp, const MeshInterface& mesh) {
    long max_gid = 0;
    long count = 0;
    for (int i = 0; i < mesh.nodeCount(); i++) {
        max_gid = std::max(mesh.globalNodeId(i), max_gid);
    }
    count += mesh.nodeCount();
    max_gid = mp.ParallelMax(max_gid);
    count = mp.ParallelSum(count);
    if (count == 0) {
        return 0;
    }
    return max_gid + 1;
}
long globalCellCount(MessagePasser mp, const MeshInterface& mesh) {
    long max_gid = 0;
    long count = 0;
    for (int i = 0; i < mesh.cellCount(); i++) {
        max_gid = std::max(mesh.globalCellId(i), max_gid);
    }
    count += mesh.cellCount();
    max_gid = mp.ParallelMax(max_gid);
    count = mp.ParallelSum(count);
    if (count == 0) {
        return 0;
    }
    return max_gid + 1;
}
long globalCellCount(MessagePasser mp,
                     const MeshInterface& mesh,
                     MeshInterface::CellType cell_type) {
    if (mp.ParallelMax(mesh.cellCount(cell_type)) == 0) return 0;

    long count = 0;
    for (int i = 0; i < mesh.cellCount(); i++) {
        if (cell_type == mesh.cellType(i) and mesh.cellOwner(i) == mesh.partitionId()) count++;
    }
    return mp.ParallelSum(count);
}
bool is2D(MessagePasser mp, const MeshInterface& mesh) {
    return maxCellDimensionality(mp, mesh) == 2;
}
bool isSimplex(const MessagePasser& mp, const MeshInterface& mesh) {
    long non_simplex_cells = 0;
    if (is2D(mp, mesh)) {
        non_simplex_cells = mesh.cellCount(MeshInterface::QUAD_4);
    } else {
        auto global_cell_count = globalCellCount(mp, mesh);
        auto global_bar_count = globalCellCount(mp, mesh, MeshInterface::BAR_2);
        auto global_tri_count = globalCellCount(mp, mesh, MeshInterface::TRI_3);
        auto global_tet_count = globalCellCount(mp, mesh, MeshInterface::TETRA_4);
        non_simplex_cells =
            global_cell_count - global_bar_count - global_tri_count - global_tet_count;
    }

    return mp.ParallelSum(non_simplex_cells) == 0;
}
std::vector<int> getCellOwners(const MeshInterface& mesh) {
    std::vector<int> owners(mesh.cellCount());
    for (int c = 0; c < mesh.cellCount(); c++) {
        auto o = mesh.cellOwner(c);
        owners[c] = o;
    }
    return owners;
}
std::vector<long> getGlobalNodeIds(const MeshInterface& mesh) {
    std::vector<long> gids(mesh.nodeCount());
    for (int n = 0; n < mesh.nodeCount(); n++) {
        auto g = mesh.globalNodeId(n);
        gids[n] = g;
    }
    return gids;
}
std::vector<long> getGlobalCellIds(const MeshInterface& mesh) {
    std::vector<long> gids(mesh.cellCount());
    for (int c = 0; c < mesh.cellCount(); c++) {
        auto g = mesh.globalCellId(c);
        gids[c] = g;
    }
    return gids;
}
std::vector<int> getNodeOwners(const MeshInterface& mesh) {
    std::vector<int> owners(mesh.nodeCount());
    for (int n = 0; n < mesh.nodeCount(); n++) {
        auto o = mesh.nodeOwner(n);
        owners[n] = o;
    }
    return owners;
}
int maxCellDimensionality(MessagePasser mp, const MeshInterface& mesh) {
    int max_dimensionality = -1;
    for (int cell_id = 0; cell_id < mesh.cellCount(); ++cell_id) {
        max_dimensionality =
            std::max(max_dimensionality, mesh.cellTypeDimensionality(mesh.cellType(cell_id)));
        if (max_dimensionality == 3) break;
    }
    return mp.ParallelMax(max_dimensionality);
}
}