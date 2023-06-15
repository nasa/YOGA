#include "CompositeMeshBuilder.h"
#include <parfait/SyncField.h>
#include <parfait/SyncPattern.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/MeshHelpers.h>

using namespace inf;

std::set<int> uniqueTagsOnMesh(MessagePasser mp, const MeshInterface& mesh) {
    std::set<int> unique_surface_tags;
    for (int i = 0; i < mesh.cellCount(); i++) {
        if (mesh.is2DCell(i)) {
            unique_surface_tags.insert(mesh.cellTag(i));
        }
    }
    return mp.ParallelUnion(unique_surface_tags);
}

std::vector<std::map<int, int>> CompositeMeshBuilder::calcMapOfNewSurfaceCellTags(
    std::vector<std::shared_ptr<MeshInterface>> meshes, MessagePasser mp) {
    std::vector<std::map<int, int>> mesh_tag_old_to_new(meshes.size());

    int next_tag = 1;
    for (int m = 0; m < int(meshes.size()); m++) {
        auto mesh_tags = uniqueTagsOnMesh(mp, *meshes[m]);
        for (auto t : mesh_tags) mesh_tag_old_to_new.at(m)[t] = next_tag++;
    }

    return mesh_tag_old_to_new;
}

std::vector<long> calcGlobalNodeIdOffsets(std::vector<std::shared_ptr<MeshInterface>> meshes,
                                          MessagePasser mp) {
    std::vector<long> offsets(meshes.size(), 0);
    for (int m = 0; m < int(meshes.size()) - 1; m++) {
        offsets[m + 1] = offsets[m] + inf::globalNodeCount(mp, *meshes[m]);
    }
    return offsets;
}
std::vector<long> calcGlobalCellIdOffsets(std::vector<std::shared_ptr<MeshInterface>> meshes,
                                          MessagePasser mp) {
    std::vector<long> offsets(meshes.size(), 0);
    for (int m = 0; m < int(meshes.size()) - 1; m++) {
        offsets[m + 1] = offsets[m] + inf::globalCellCount(mp, *meshes[m]);
    }
    return offsets;
}
std::vector<int> calcLocalNodeIdOffsets(std::vector<std::shared_ptr<MeshInterface>> meshes) {
    std::vector<int> local_node_id_offset(meshes.size(), 0);
    for (int m = 0; m < int(meshes.size()) - 1; m++) {
        local_node_id_offset[m + 1] = local_node_id_offset[m] + meshes[m]->nodeCount();
    }
    return local_node_id_offset;
}
std::shared_ptr<TinfMesh> CompositeMeshBuilder::createCompositeMesh(
    MessagePasser mp, std::vector<std::shared_ptr<MeshInterface>> meshes) {
    auto local_node_offsets = calcLocalNodeIdOffsets(meshes);
    auto global_node_offsets = calcGlobalNodeIdOffsets(meshes, mp);
    auto global_cell_offsets = calcGlobalCellIdOffsets(meshes, mp);

    std::vector<std::map<int, int>> surface_cell_tags_map = calcMapOfNewSurfaceCellTags(meshes, mp);
    TinfMeshData combined_mesh;
    for (auto& m : meshes) {
        for (int n = 0; n < m->nodeCount(); n++) {
            auto p = m->node(n);
            combined_mesh.points.push_back(p);
        }
    }
    for (auto& m : meshes) {
        for (int n = 0; n < m->nodeCount(); n++) {
            auto owner = m->nodeOwner(n);
            combined_mesh.node_owner.push_back(owner);
        }
    }
    for (int m = 0; m < int(meshes.size()); m++) {
        auto mesh = meshes[m];
        for (int n = 0; n < mesh->nodeCount(); n++) {
            auto gid = mesh->globalNodeId(n) + global_node_offsets[m];
            combined_mesh.global_node_id.push_back(gid);
        }
    }
    for (int m = 0; m < int(meshes.size()); m++) {
        auto mesh = meshes[m];
        for (int c = 0; c < mesh->cellCount(); c++) {
            auto gid = mesh->globalCellId(c) + global_cell_offsets[m];
            auto type = mesh->cellType(c);
            combined_mesh.global_cell_id[type].push_back(gid);
        }
    }
    for (auto mesh : meshes) {
        for (int c = 0; c < mesh->cellCount(); c++) {
            auto owner = mesh->cellOwner(c);
            auto type = mesh->cellType(c);
            combined_mesh.cell_owner[type].push_back(owner);
        }
    }
    for (int m = 0; m < int(meshes.size()); m++) {
        auto mesh = meshes[m];
        std::vector<int> cell_nodes(8);
        for (int c = 0; c < mesh->cellCount(); c++) {
            auto type = mesh->cellType(c);
            int length = mesh->cellTypeLength(type);
            cell_nodes.resize(length);
            mesh->cell(c, cell_nodes.data());
            for (auto& n : cell_nodes) {
                n += local_node_offsets[m];
                combined_mesh.cells[type].push_back(n);
            }
        }
    }
    for (int m = 0; m < int(meshes.size()); m++) {
        auto mesh = meshes[m];
        for (int c = 0; c < mesh->cellCount(); c++) {
            auto type = mesh->cellType(c);
            auto tag = mesh->cellTag(c);
            if (MeshInterface::is2DCellType(type))
                combined_mesh.cell_tags[type].push_back(surface_cell_tags_map[m].at(tag));
            else
                combined_mesh.cell_tags[type].push_back(tag);
        }
    }
    return std::make_shared<TinfMesh>(std::move(combined_mesh), mp.Rank());
}
