#include "MeshMirror.h"
#include "Extract.h"
#include "FilterFactory.h"
#include "MeshMover.h"
#include "CompositeMeshBuilder.h"
#include "MeshHelpers.h"

Parfait::Point<double> inf::MeshMirror::getBoundaryPointWithTag(const MessagePasser& mp,
                                                                const inf::MeshInterface& mesh,
                                                                int tag) {
    auto boundary_cell_dimensionality = maxCellDimensionality(mp, mesh) - 1;
    long gid = 0;
    int local_id = 0;
    std::vector<int> cell_nodes;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        if (mesh.cellTag(cell_id) == tag and
            mesh.cellDimensionality(cell_id) == boundary_cell_dimensionality) {
            mesh.cell(cell_id, cell_nodes);
            gid = mesh.globalNodeId(cell_nodes.front());
            local_id = cell_nodes.front();
            break;
        }
    }
    long max_gid = mp.ParallelMax(gid);
    double uninitialized = std::numeric_limits<double>::max();
    Parfait::Point<double> p{uninitialized, uninitialized, uninitialized};
    if (gid == max_gid) {
        mesh.nodeCoordinate(local_id, p.data());
    }
    p[0] = mp.ParallelMin(p[0]);
    p[1] = mp.ParallelMin(p[1]);
    p[2] = mp.ParallelMin(p[2]);

    return p;
}
double inf::MeshMirror::getDistanceFromMirrorSurfaceToPlane(
    const MessagePasser mp,
    const inf::MeshInterface& mesh,
    const std::set<int>& tags,
    const Parfait::Point<double>& plane_normal) {
    int first_tag = *tags.begin();
    auto p = getBoundaryPointWithTag(mp, mesh, first_tag);
    return -Parfait::Point<double>::dot(p, plane_normal);
}

inf::MeshMirror::MirroredMeshZipper inf::MeshMirror::mirrorViaTags(
    const MessagePasser& mp,
    std::shared_ptr<MeshInterface> mesh,
    const std::set<int>& mirror_tags) {
    auto normal = inf::extractAverageNormal(mp, *mesh, mirror_tags);
    normal.normalize();
    double offset = getDistanceFromMirrorSurfaceToPlane(mp, *mesh, mirror_tags, normal);
    return mirrorViaTags(mp, mesh, mirror_tags, normal, offset);
}

inf::MeshMirror::MirroredMeshZipper inf::MeshMirror::mirrorViaTags(
    const MessagePasser& mp,
    std::shared_ptr<MeshInterface> mesh,
    const std::set<int>& mirror_tags,
    const Parfait::Point<double>& plane_normal,
    double plane_offset) {
    std::set<int> nodes_to_remove_from_reflected_mesh;
    int dim = inf::maxCellDimensionality(mp, *mesh);
    for (int tag : mirror_tags) {
        auto nodes_with_tag = inf::extractNodeIdsViaDimensionalityAndTag(*mesh, tag, dim - 1);
        nodes_to_remove_from_reflected_mesh.insert(nodes_with_tag.begin(), nodes_with_tag.end());
    }

    auto selected_tags = inf::extractTags(*mesh);
    for (int tag : mirror_tags) std::remove(selected_tags.begin(), selected_tags.end(), tag);

    auto tag_filter = inf::FilterFactory::tagFilter(mp.getCommunicator(), mesh, selected_tags);

    mesh = tag_filter->getMesh();

    auto mirrored = inf::MeshMover::reflect(mesh, plane_normal, plane_offset);
    MirroredMeshZipper zipper(mp, *mesh, *mirrored, nodes_to_remove_from_reflected_mesh);
    return zipper;
}

long countRemovedNodesLessThan(const std::vector<long>& removed_gids, long gid) {
    auto lb = std::lower_bound(removed_gids.begin(), removed_gids.end(), gid);
    return std::distance(removed_gids.begin(), lb);
}

inf::MeshMirror::MirroredMeshZipper::MirroredMeshZipper(
    const MessagePasser& mp,
    const inf::MeshInterface& mesh,
    const inf::MeshInterface& reflected_mesh,
    const std::set<int>& nodes_to_remove_from_reflected_mesh) {
    TinfMesh combined_mesh(mesh);
    auto& mesh_data = combined_mesh.mesh;
    std::vector<int> reflected_local_node_id_to_combined(reflected_mesh.nodeCount());

    long total_nodes_in_orig_mesh = globalNodeCount(mp, mesh);
    std::set<long> gids_removed;
    for (int id : nodes_to_remove_from_reflected_mesh)
        gids_removed.insert(reflected_mesh.globalNodeId(id));
    gids_removed = mp.ParallelUnion(gids_removed);
    std::vector<long> gids_removed_v(gids_removed.begin(), gids_removed.end());

    int combined_node_count = reflected_mesh.nodeCount();
    for (int i = 0; i < reflected_mesh.nodeCount(); i++) {
        auto p = reflected_mesh.node(i);
        long gid = reflected_mesh.globalNodeId(i);
        long shifted_gid = gid + total_nodes_in_orig_mesh;
        shifted_gid -= countRemovedNodesLessThan(gids_removed_v, gid);
        if (nodes_to_remove_from_reflected_mesh.count(i) == 0) {
            mesh_data.points.push_back(p);
            mesh_data.global_node_id.push_back(shifted_gid);
            reflected_local_node_id_to_combined[i] = combined_node_count++;
        } else {
            reflected_local_node_id_to_combined[i] = i;
        }
    }

    combined_local_node_id_to_original.resize(mesh_data.points.size(), -1);
    for (int i = 0; i < mesh.nodeCount(); i++) combined_local_node_id_to_original[i] = i;
    for (int i = 0; i < int(reflected_local_node_id_to_combined.size()); i++) {
        int combined_id = reflected_local_node_id_to_combined[i];
        int original = i;
        combined_local_node_id_to_original[combined_id] = original;
    }

    for (int i = 0; i < reflected_mesh.nodeCount(); i++) {
        int owning_rank = reflected_mesh.nodeOwner(i);
        if (nodes_to_remove_from_reflected_mesh.count(i) == 0) {
            mesh_data.node_owner.push_back(owning_rank);
        }
    }

    long cells_in_orig_mesh = globalCellCount(mp, reflected_mesh);
    for (int i = 0; i < reflected_mesh.cellCount(); i++) {
        auto gid = reflected_mesh.globalCellId(i) + cells_in_orig_mesh;
        auto type = reflected_mesh.cellType(i);
        mesh_data.global_cell_id[type].push_back(gid);
    }

    for (int i = 0; i < reflected_mesh.cellCount(); i++) {
        auto owner = reflected_mesh.cellOwner(i);
        auto type = reflected_mesh.cellType(i);
        mesh_data.cell_owner[type].push_back(owner);
    }

    std::vector<int> cell_nodes(8);
    for (int i = 0; i < reflected_mesh.cellCount(); i++) {
        reflected_mesh.cell(i, cell_nodes);
        auto type = reflected_mesh.cellType(i);
        for (int n : cell_nodes) {
            n = reflected_local_node_id_to_combined[n];
            mesh_data.cells[type].push_back(n);
        }
    }

    for (int i = 0; i < reflected_mesh.cellCount(); i++) {
        auto type = reflected_mesh.cellType(i);
        auto tag = reflected_mesh.cellTag(i);
        mesh_data.cell_tags[type].push_back(tag);
    }

    zipped_mesh = std::make_shared<TinfMesh>(mesh_data, mesh.partitionId());
}
std::shared_ptr<inf::MeshInterface> inf::MeshMirror::MirroredMeshZipper::getMesh() const {
    return zipped_mesh;
}
int inf::MeshMirror::MirroredMeshZipper::previousNodeId(int id) {
    return combined_local_node_id_to_original.at(id);
}
