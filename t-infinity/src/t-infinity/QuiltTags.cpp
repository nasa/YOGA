#include "QuiltTags.h"
#include "Cell.h"
#include "MeshHelpers.h"
std::map<int, int> inf::quiltTagsDontCompact(const std::vector<std::vector<int>>& tags_to_merge,
                                             const std::set<int>& current_tags) {
    std::map<int, int> old_to_new;

    for (auto& tags : tags_to_merge) {
        if (tags.size() == 0) continue;

        int new_tag = *tags.begin();
        for (auto t : tags) {
            old_to_new[t] = new_tag;
        }
    }

    for (auto& t : current_tags) {
        if (old_to_new.count(t) == 0) old_to_new[t] = t;
    }

    return old_to_new;
}
std::map<int, int> inf::quiltTagsAndCompact(const std::vector<std::vector<int>>& tags_to_merge,
                                            const std::set<int>& current_tags) {
    auto old_to_new = quiltTagsDontCompact(tags_to_merge, current_tags);

    std::set<int> new_tags;
    for (auto& pair : old_to_new) {
        auto t = pair.second;
        new_tags.insert(t);
    }

    std::map<int, int> new_to_flat;

    int flat = 1;
    for (auto& t : new_tags) {
        new_to_flat[t] = flat++;
    }

    for (auto& pair : old_to_new) {
        int old_t = pair.first;
        int new_t = pair.second;
        flat = new_to_flat[new_t];
        old_to_new[old_t] = flat;
    }

    return old_to_new;
}
std::map<int, int> inf::quiltTagsAndCompact(MessagePasser mp,
                                            inf::TinfMesh& m,
                                            std::vector<std::vector<int>> tags_to_merge) {
    auto surface_element_dimensionality = maxCellDimensionality(mp, m) - 1;
    auto current_tags = extractAllTagsWithDimensionality(mp, m, surface_element_dimensionality);
    mp_rootprint("Current mesh has tags: %s\n", Parfait::to_string(current_tags).c_str());
    auto old_to_new = quiltTagsAndCompact(tags_to_merge, current_tags);
    mp_rootprint("Quilting boundary tags:\n");
    for (auto& pair : old_to_new) {
        mp_rootprint("  old tag %d is now %d\n", pair.first, pair.second);
    }

    remapSurfaceTags(m, old_to_new);
    return old_to_new;
}
std::map<int, int> inf::quiltTagsAndDontCompact(MessagePasser mp,
                                                inf::TinfMesh& m,
                                                std::vector<std::vector<int>> tags_to_merge) {
    auto surface_element_dimensionality = maxCellDimensionality(mp, m) - 1;
    auto current_tags = extractAllTagsWithDimensionality(mp, m, surface_element_dimensionality);
    mp_rootprint("Current mesh has tags: %s\n", Parfait::to_string(current_tags).c_str());
    auto old_to_new = quiltTagsDontCompact(tags_to_merge, current_tags);
    mp_rootprint("Quilting boundary tags:\n");
    for (auto& pair : old_to_new) {
        mp_rootprint("  old tag %d is now %d\n", pair.first, pair.second);
    }

    remapSurfaceTags(m, old_to_new);
    return old_to_new;
}

int findBestTag(const std::map<int, Parfait::Point<double>>& tag_to_normal,
                Parfait::Point<double> normal) {
    normal.normalize();
    int best_tag = tag_to_normal.begin()->first;
    double biggest_dot = -2.0;
    for (auto& pair : tag_to_normal) {
        auto n2 = pair.second;
        n2.normalize();
        auto d = Parfait::Point<double>::dot(normal, n2);
        if (d > biggest_dot) {
            biggest_dot = d;
            best_tag = pair.first;
        }
    }
    return best_tag;
}

void inf::relabelSurfaceTagsByNearestNormal(
    inf::TinfMesh& mesh,
    const std::map<int, Parfait::Point<double>>& tag_to_normal,
    const std::set<int>& tags_to_overwrite) {
    for (int c = 0; c < mesh.cellCount(); c++) {
        auto type = mesh.cellType(c);
        int tag = mesh.cellTag(c);
        if (not tags_to_overwrite.count(tag)) continue;
        if (not inf::TinfMesh::is2DCellType(type)) continue;

        auto normal = inf::Cell(mesh, c).faceAreaNormal(0);
        auto t = findBestTag(tag_to_normal, normal);
        int type_index = mesh.cell_id_to_type_and_local_id[c].second;
        mesh.mesh.cell_tags[type][type_index] = t;
    }
}
std::map<int, Parfait::Point<double>> inf::getCartesianNormalTags(int new_tag_start) {
    static std::map<int, Parfait::Point<double>> tags_to_normals;
    tags_to_normals[new_tag_start + 0] = {0, 0, -1};
    tags_to_normals[new_tag_start + 1] = {0, -1, 0};
    tags_to_normals[new_tag_start + 2] = {1, 0, 0};
    tags_to_normals[new_tag_start + 3] = {0, 1, 0};
    tags_to_normals[new_tag_start + 4] = {-1, 0, 0};
    tags_to_normals[new_tag_start + 5] = {0, 0, 1};
    return tags_to_normals;
}
void inf::compactTags(MessagePasser mp, inf::TinfMesh& m) { quiltTagsAndCompact(mp, m, {}); }
void inf::remapSurfaceTags(inf::TinfMesh& m, const std::map<int, int>& old_to_new_tags) {
    for (auto& pair : m.mesh.cell_tags) {
        auto type = pair.first;
        if (inf::MeshInterface::is2DCellType(type)) {
            for (auto& t : pair.second) {
                if (old_to_new_tags.count(t) == 0) {
                    PARFAIT_EXIT("Cannot map old tag " + std::to_string(t));
                }
                t = old_to_new_tags.at(t);
            }
        }
    }
}
