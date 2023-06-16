#pragma once
#include <memory>
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/MeshInterface.h>
#include "TinfMesh.h"
#include "Extract.h"

namespace inf {

template <typename T>
std::tuple<std::vector<long>, std::vector<T>> extractOwnedGlobalNodeIdsAndField(
    const inf::MeshInterface& mesh, const std::vector<T>& field) {
    std::vector<T> output_field;
    std::vector<long> gids;
    int node_count = mesh.nodeCount();
    for (int node = 0; node < node_count; node++) {
        if (mesh.nodeOwner(node) == mesh.partitionId()) {
            auto d = field[node];
            output_field.push_back(d);
            gids.push_back(mesh.globalNodeId(node));
        }
    }
    return {gids, output_field};
}
inline std::tuple<std::vector<long>, std::vector<Parfait::Point<double>>>
extractOwnedGlobalNodeIdsAndPoints(const inf::MeshInterface& mesh) {
    return extractOwnedGlobalNodeIdsAndField(mesh, extractPoints(mesh));
}

inline std::vector<long> extractOwnedCellGlobalIds(const inf::MeshInterface& mesh,
                                                   inf::MeshInterface::CellType type) {
    std::vector<long> gids;
    int cell_count = mesh.cellCount();
    for (int c = 0; c < cell_count; c++) {
        auto t = mesh.cellType(c);
        if (t == type and mesh.cellOwner(c) == mesh.partitionId())
            gids.push_back(mesh.globalCellId(c));
    }
    return gids;
}

inline std::vector<int> extractOwnedCellTags(const inf::MeshInterface& mesh,
                                             inf::MeshInterface::CellType type) {
    std::vector<int> tags;
    int cell_count = mesh.cellCount();
    for (int c = 0; c < cell_count; c++) {
        auto t = mesh.cellType(c);
        if (t == type and mesh.cellOwner(c) == mesh.partitionId()) tags.push_back(mesh.cellTag(c));
    }
    return tags;
}

inline std::vector<int> extractOwnedCellGlobalNodesAsInts(const inf::MeshInterface& mesh,
                                                          inf::MeshInterface::CellType type) {
    std::vector<int> node_ids;
    int cell_count = mesh.cellCount();
    int length = mesh.cellTypeLength(type);
    std::vector<int> temp(length);
    for (int c = 0; c < cell_count; c++) {
        auto t = mesh.cellType(c);
        if (t == type and mesh.cellOwner(c) == mesh.partitionId()) {
            mesh.cell(c, temp.data());
            for (auto n : temp) {
                node_ids.push_back(static_cast<int>(mesh.globalNodeId(n)));
            }
        }
    }
    return node_ids;
}

inline std::vector<inf::MeshInterface::CellType> extractTypes(const inf::MeshInterface& mesh) {
    std::set<inf::MeshInterface::CellType> my_types;
    int cell_count = mesh.cellCount();
    for (int c = 0; c < cell_count; c++) {
        my_types.insert(mesh.cellType(c));
    }
    return std::vector<inf::MeshInterface::CellType>(std::begin(my_types), std::end(my_types));
}

inline std::vector<inf::MeshInterface::CellType> gatherTypes(
    MessagePasser mp, const std::vector<inf::MeshInterface::CellType>& my_types) {
    auto all_types_from_ranks = mp.Gather(my_types);
    std::set<inf::MeshInterface::CellType> unique;
    for (auto& types : all_types_from_ranks) {
        for (auto t : types) {
            unique.insert(t);
        }
    }
    return std::vector<inf::MeshInterface::CellType>(std::begin(unique), std::end(unique));
}

inline std::vector<inf::MeshInterface::CellType> gatherTypes(MessagePasser mp,
                                                             const inf::MeshInterface& mesh) {
    return gatherTypes(mp, extractTypes(mesh));
}

template <typename T>
inline std::vector<T> flatten(const std::vector<std::vector<T>>& in) {
    std::vector<T> out;
    for (auto& outer : in)
        for (auto& t : outer) out.push_back(t);
    return out;
}

inline std::vector<Parfait::Point<double>> gatherMeshPoints(const inf::MeshInterface& mesh,
                                                            MPI_Comm comm,
                                                            int root) {
    MessagePasser mp(comm);
    std::vector<Parfait::Point<double>> points;
    std::vector<long> gids;
    std::tie(gids, points) = extractOwnedGlobalNodeIdsAndPoints(mesh);
    PARFAIT_ASSERT(gids.size() == points.size(), "gid points mismatch");
    return mp.GatherByIds(points, gids, root);
}

inline std::shared_ptr<inf::TinfMesh> gatherMesh(const inf::MeshInterface& mesh,
                                                 MPI_Comm comm,
                                                 int root) {
    MessagePasser mp(comm);
    inf::TinfMeshData mesh_data;
    {
        std::vector<long> gids;
        std::vector<Parfait::Point<double>> points;
        std::tie(gids, points) = extractOwnedGlobalNodeIdsAndPoints(mesh);
        mesh_data.points = mp.GatherByIds(points, gids, root);
        mesh_data.global_node_id = mp.GatherByIds(gids, gids, root);
        mesh_data.node_owner = std::vector<int>(mesh_data.global_node_id.size(), root);
    }
    auto all_types = gatherTypes(mp, mesh);

    for (auto type : all_types) {
        auto tags = extractOwnedCellTags(mesh, type);
        auto cell_global_node_ids_as_ints = extractOwnedCellGlobalNodesAsInts(mesh, type);
        auto all_tags = mp.Gather(tags, root);
        mesh_data.cell_tags[type] = flatten(all_tags);

        auto all_cell_nodes = mp.Gather(cell_global_node_ids_as_ints, root);
        mesh_data.cells[type] = flatten(all_cell_nodes);

        auto all_gids = mp.Gather(extractOwnedCellGlobalIds(mesh, type), root);
        mesh_data.global_cell_id[type] = flatten(all_gids);
        mesh_data.cell_owner[type] = std::vector<int>(mesh_data.global_cell_id[type].size(), root);
    }

    return std::make_shared<inf::TinfMesh>(mesh_data, mp.Rank());
}
}