#include "Extract.h"
#include <parfait/Normals.h>
#include <parfait/Metrics.h>
#include "MeshHelpers.h"
#include "Cell.h"

std::set<int> inf::extractTagsWithDimensionality(const inf::MeshInterface& mesh,
                                                 int dimensionality) {
    std::set<int> tags;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.cellDimensionality(c) == dimensionality) {
            int tag = mesh.cellTag(c);
            tags.insert(tag);
        }
    }
    return tags;
}
std::set<int> inf::extractAllTagsWithDimensionality(MessagePasser mp,
                                                    const inf::MeshInterface& mesh,
                                                    int dimensionality) {
    auto tags = extractTagsWithDimensionality(mesh, dimensionality);
    auto all_tags = mp.ParallelUnion(tags);
    return all_tags;
}
void addFacets(const inf::MeshInterface& mesh, int cell_id, std::vector<Parfait::Facet>& facets) {
    std::array<int, 6> cell_nodes;
    auto type = mesh.cellType(cell_id);
    mesh.cell(cell_id, cell_nodes.data());
    auto a = mesh.node(cell_nodes[0]);
    auto b = mesh.node(cell_nodes[1]);
    auto c = mesh.node(cell_nodes[2]);
    Parfait::Point<double> d, e, f;

    if (type == inf::MeshInterface::TRI_3) {
        facets.emplace_back(a, b, c);
        facets.back().tag = mesh.cellTag(cell_id);
        return;
    }

    if (type == inf::MeshInterface::QUAD_4) {
        d = mesh.node(cell_nodes[3]);
        facets.emplace_back(a, b, c);
        facets.back().tag = mesh.cellTag(cell_id);
        facets.emplace_back(c, d, a);
        facets.back().tag = mesh.cellTag(cell_id);
        return;
    }

    if (type == inf::MeshInterface::TRI_6) {
        d = mesh.node(cell_nodes[3]);
        e = mesh.node(cell_nodes[4]);
        f = mesh.node(cell_nodes[5]);
        facets.emplace_back(a, d, f);
        facets.back().tag = mesh.cellTag(cell_id);
        facets.emplace_back(d, b, e);
        facets.back().tag = mesh.cellTag(cell_id);
        facets.emplace_back(d, e, f);
        facets.back().tag = mesh.cellTag(cell_id);
        facets.emplace_back(f, e, c);
        facets.back().tag = mesh.cellTag(cell_id);
    }
}
std::vector<int> inf::extractOwnedCellIdsInTagSet(const inf::MeshInterface& mesh,
                                                  const std::set<int>& tag_set,
                                                  int dimensionality) {
    std::vector<int> surface_cells_in_tag;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.ownedCell(c) and mesh.cellDimensionality(c) == dimensionality) {
            if (tag_set.count(mesh.cellTag(c)) == 1) {
                surface_cells_in_tag.push_back(c);
            }
        }
    }
    return surface_cells_in_tag;
}
std::vector<int> inf::extractOwnedCellIdsOnTag(const inf::MeshInterface& mesh,
                                               int tag,
                                               int dimensionality) {
    std::vector<int> surface_cells_in_tag;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.ownedCell(c) and mesh.cellDimensionality(c) == dimensionality) {
            if (mesh.cellTag(c) == tag) surface_cells_in_tag.push_back(c);
        }
    }
    return surface_cells_in_tag;
}
std::vector<int> inf::extractCellIdsOnTag(const inf::MeshInterface& mesh,
                                          int tag,
                                          int dimensionality) {
    std::vector<int> surface_cells_in_tag;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.cellDimensionality(c) == dimensionality) {
            if (mesh.cellTag(c) == tag) surface_cells_in_tag.push_back(c);
        }
    }
    return surface_cells_in_tag;
}
std::vector<Parfait::Facet> inf::extractOwned2DFacets(MessagePasser mp,
                                                      const inf::MeshInterface& mesh) {
    std::vector<Parfait::Facet> facets;
    facets.reserve(mesh.cellCount(MeshInterface::TRI_3) + mesh.cellCount(MeshInterface::QUAD_4));
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.cellOwner(c) == mp.Rank()) {
            if (mesh.is2DCell(c)) {
                addFacets(mesh, c, facets);
            }
        }
    }
    return facets;
}
std::vector<Parfait::Facet> inf::extractOwned2DFacets(MessagePasser mp,
                                                      const inf::MeshInterface& mesh,
                                                      const std::set<int>& tags) {
    std::vector<Parfait::Facet> facets;
    facets.reserve(mesh.cellCount(MeshInterface::TRI_3) + mesh.cellCount(MeshInterface::QUAD_4));
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.cellOwner(c) == mp.Rank()) {
            if (mesh.is2DCell(c)) {
                int tag = mesh.cellTag(c);
                if (tags.count(tag)) {
                    addFacets(mesh, c, facets);
                }
            }
        }
    }
    return facets;
}
std::vector<Parfait::Facet> inf::extractOwned2DFacets(MessagePasser mp,
                                                      const inf::MeshInterface& mesh,
                                                      const std::set<std::string>& tag_names) {
    std::set<int> tags;
    for (const auto& n : tag_names) {
        auto tag_set_n = inf::extractTagsByName(mp, mesh, n);
        tags.insert(tag_set_n.begin(), tag_set_n.end());
    }
    return extractOwned2DFacets(mp, mesh, tags);
}
std::tuple<std::vector<Parfait::Facet>, std::vector<int>> inf::extractOwned2DFacetsAndTheirTags(
    MessagePasser mp, const inf::MeshInterface& mesh, const std::set<int>& tags) {
    std::vector<Parfait::Facet> facets;
    std::vector<int> facet_tags;
    int num_facets = mesh.cellCount(MeshInterface::TRI_3) + mesh.cellCount(MeshInterface::QUAD_4);
    facets.reserve(num_facets);
    facet_tags.reserve(num_facets);
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.cellOwner(c) == mp.Rank()) {
            auto type = mesh.cellType(c);
            if (mesh.is2DCellType(type)) {
                int tag = mesh.cellTag(c);
                if (tags.count(tag)) {
                    size_t current_facet_count = facets.size();
                    addFacets(mesh, c, facets);
                    size_t f_count = facets.size() - current_facet_count;
                    for (size_t i = 0; i < f_count; i++) {
                        facet_tags.push_back(tag);
                    }
                }
            }
        }
    }
    if (facets.size() != facet_tags.size()) {
        PARFAIT_THROW("We should have a tag for each facet.");
    }
    return {facets, facet_tags};
}
std::vector<Parfait::Point<double>> inf::extractPoints(const inf::MeshInterface& mesh,
                                                       const std::vector<int>& node_ids) {
    std::vector<Parfait::Point<double>> points;
    points.reserve(node_ids.size());
    for (int i : node_ids) {
        points.push_back(mesh.node(i));
    }
    return points;
}
std::vector<Parfait::Point<double>> inf::extractOwnedPoints(const inf::MeshInterface& mesh) {
    std::vector<Parfait::Point<double>> points;
    points.reserve(0.8 * mesh.nodeCount());
    for (int i = 0; i < mesh.nodeCount(); i++) {
        if (mesh.ownedNode(i)) points.push_back(mesh.node(i));
    }
    return points;
}
std::vector<int> inf::extractNodeIdsViaDimensionalityAndTag(const inf::MeshInterface& mesh,
                                                            int tag,
                                                            int dimensionality) {
    std::set<int> node_ids;

    std::vector<int> cell_nodes;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.cellDimensionality(c) == dimensionality) {
            auto t = mesh.cellTag(c);
            if (t != tag) continue;
            mesh.cell(c, cell_nodes);
            for (auto n : cell_nodes) {
                node_ids.insert(n);
            }
        }
    }
    return {node_ids.begin(), node_ids.end()};
}
std::vector<int> inf::extractOwnedNodeIdsViaDimensionalityAndTag(const inf::MeshInterface& mesh,
                                                                 int tag,
                                                                 int dimensionality) {
    std::set<int> node_ids;

    std::vector<int> cell_nodes;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.cellDimensionality(c) == dimensionality and mesh.cellTag(c) == tag) {
            mesh.cell(c, cell_nodes);
            for (auto n : cell_nodes) {
                if (mesh.ownedNode(n)) {
                    node_ids.insert(n);
                }
            }
        }
    }
    return {node_ids.begin(), node_ids.end()};
}
std::vector<Parfait::Point<double>> inf::extractPoints(const inf::MeshInterface& mesh) {
    std::vector<Parfait::Point<double>> points(mesh.nodeCount());
    for (int i = 0; i < mesh.nodeCount(); i++) {
        points[i] = mesh.node(i);
    }
    return points;
}
std::vector<Parfait::Point<double>> inf::extractCellCentroids(const inf::MeshInterface& mesh) {
    std::vector<Parfait::Point<double>> points(mesh.cellCount());
    for (int c = 0; c < mesh.cellCount(); c++) {
        auto cell = inf::Cell(mesh, c);
        points[c] = cell.averageCenter();
    }
    return points;
}
std::vector<int> inf::extractTags(const inf::MeshInterface& mesh) {
    std::vector<int> tags(mesh.cellCount());
    for (int c = 0; c < mesh.cellCount(); c++) {
        tags[c] = mesh.cellTag(c);
    }
    return tags;
}
std::vector<int> inf::extractCellOwners(const inf::MeshInterface& mesh) {
    std::vector<int> owners(mesh.cellCount());
    for (int c = 0; c < mesh.cellCount(); c++) {
        owners[c] = mesh.cellOwner(c);
    }
    return owners;
}
std::vector<int> inf::extractNodeOwners(const inf::MeshInterface& mesh) {
    std::vector<int> owners(mesh.nodeCount());
    for (int n = 0; n < mesh.nodeCount(); n++) {
        owners[n] = mesh.nodeOwner(n);
    }
    return owners;
}
Parfait::Point<double> inf::extractAverageNormal(const MessagePasser& mp,
                                                 const inf::MeshInterface& mesh,
                                                 const std::set<int>& tags) {
    auto surface_cell_dimensionality = maxCellDimensionality(mp, mesh) - 1;
    Parfait::Point<double> average_normal{};
    long count = 0;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.ownedCell(c) and mesh.cellDimensionality(c) == surface_cell_dimensionality) {
            if (tags.count(mesh.cellTag(c)) != 0) {
                if (surface_cell_dimensionality == 1) {
                    std::array<int, 2> cell_nodes{};
                    mesh.cell(c, cell_nodes.data());
                    auto normal = Parfait::Metrics::XYLineNormal(mesh.node(cell_nodes[0]),
                                                                 mesh.node(cell_nodes[1]));
                    normal.normalize();
                    average_normal += normal;
                    count++;
                } else {
                    Cell cell(mesh, c);
                    for (int f = 0; f < cell.faceCount(); ++f) {
                        auto normal = cell.faceAreaNormal(f);
                        normal.normalize();
                        average_normal += normal;
                        count++;
                    }
                }
            }
        }
    }
    average_normal[0] = mp.ParallelSum(average_normal[0]);
    average_normal[1] = mp.ParallelSum(average_normal[1]);
    average_normal[2] = mp.ParallelSum(average_normal[2]);
    average_normal /= double(mp.ParallelSum(count));
    return average_normal;
}
std::vector<bool> inf::extractDoOwnCells(const inf::MeshInterface& mesh) {
    std::vector<bool> do_own(mesh.cellCount());
    for (int c = 0; c < mesh.cellCount(); c++) {
        do_own[c] = mesh.ownedCell(c);
    }
    return do_own;
}
std::vector<long> inf::extractOwnedGlobalNodeIds(const inf::MeshInterface& mesh) {
    std::vector<long> gnids;
    for (int n = 0; n < mesh.nodeCount(); n++) {
        if (mesh.ownedNode(n)) gnids.push_back(mesh.globalNodeId(n));
    }
    return gnids;
}
std::vector<long> inf::extractOwnedGlobalCellIds(const inf::MeshInterface& mesh) {
    std::vector<long> gcids;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.ownedCell(c)) gcids.push_back(mesh.globalCellId(c));
    }
    return gcids;
}
std::vector<long> inf::extractGlobalNodeIds(const inf::MeshInterface& mesh) {
    std::vector<long> gnids(mesh.nodeCount());
    for (int n = 0; n < mesh.nodeCount(); n++) {
        gnids[n] = mesh.globalNodeId(n);
    }
    return gnids;
}
std::vector<long> inf::extractGlobalCellIds(const inf::MeshInterface& mesh) {
    std::vector<long> gcids(mesh.cellCount());
    for (int c = 0; c < mesh.cellCount(); c++) {
        gcids[c] = mesh.globalCellId(c);
    }
    return gcids;
}
std::set<int> inf::extractResidentButNotOwnedNodesByDistance(
    const inf::MeshInterface& mesh, int distance, const std::vector<std::vector<int>>& n2n) {
    PARFAIT_ASSERT(distance <= 2, "Only supports up to distance-2");
    std::set<int> node_ids;
    auto addNeighbors = [&](int node_id) {
        for (int n : n2n[node_id]) {
            if (not mesh.ownedNode(n)) node_ids.insert(n);
        }
    };
    for (int node_id = 0; node_id < mesh.nodeCount(); ++node_id) {
        if (mesh.ownedNode(node_id)) {
            addNeighbors(node_id);
            if (distance == 2) {
                for (int n : n2n[node_id]) {
                    addNeighbors(n);
                }
            }
        }
    }
    return node_ids;
}
std::map<int, std::string> inf::extractTagsToNames(MessagePasser mp,
                                                   const inf::MeshInterface& mesh,
                                                   int dimensionality) {
    std::map<int, std::string> tags_to_names;
    auto tags = extractAllTagsWithDimensionality(mp, mesh, dimensionality);
    for (int t : tags) {
        tags_to_names[t] = mesh.tagName(t);
    }
    return tags_to_names;
}
std::map<std::string, std::set<int>> inf::extractNameToTags(MessagePasser mp,
                                                            const inf::MeshInterface& mesh,
                                                            int dimensionality) {
    std::map<std::string, std::set<int>> name_to_tags;
    auto tags = extractAllTagsWithDimensionality(mp, mesh, dimensionality);
    for (int t : tags) {
        name_to_tags[mesh.tagName(t)].insert(t);
    }
    return name_to_tags;
}
std::set<int> inf::extractTagsByName(MessagePasser mp,
                                     const inf::MeshInterface& mesh,
                                     std::string tag_name) {
    auto name_to_tags = inf::extractNameToTags(mp, mesh, 2);
    return name_to_tags[tag_name];
}
std::vector<Parfait::Point<double>> inf::extractSurfaceArea(const inf::MeshInterface& mesh,
                                                            int surface_cell_dimensionality) {
    std::vector<Parfait::Point<double>> surface_area(mesh.cellCount(), {0.0, 0.0, 0.0});
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.cellDimensionality(c) != surface_cell_dimensionality) continue;

        inf::Cell cell(mesh, c, surface_cell_dimensionality + 1);
        surface_area[c] = cell.faceAreaNormal(0);
    }

    return surface_area;
}
std::vector<int> inf::extractAllNodeIdsInCellsWithDimensionality(const inf::MeshInterface& mesh,
                                                                 int dimensionality) {
    std::set<int> surface_nodes;
    std::vector<int> cell_nodes;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.cellDimensionality(c) == dimensionality) {
            mesh.cell(c, cell_nodes);
            for (auto n : cell_nodes) {
                surface_nodes.insert(n);
            }
        }
    }
    return {surface_nodes.begin(), surface_nodes.end()};
}
std::set<int> inf::extract2DTags(const inf::MeshInterface& mesh) {
    return extractTagsWithDimensionality(mesh, 2);
}
std::set<int> inf::extractAll2DTags(MessagePasser mp, const inf::MeshInterface& mesh) {
    return extractAllTagsWithDimensionality(mp, mesh, 2);
}
std::map<int, std::string> inf::extract1DTagsToNames(MessagePasser mp,
                                                     const inf::MeshInterface& mesh) {
    return extractTagsToNames(mp, mesh, 1);
}
std::map<int, std::string> inf::extract2DTagsToNames(MessagePasser mp,
                                                     const inf::MeshInterface& mesh) {
    return extractTagsToNames(mp, mesh, 2);
}
std::vector<int> inf::extractNodeIdsOn2DTags(const inf::MeshInterface& mesh, int tag) {
    return extractNodeIdsViaDimensionalityAndTag(mesh, tag, 2);
}
std::vector<int> inf::extractOwnedNodeIdsOn2DTags(const inf::MeshInterface& mesh, int tag) {
    return extractOwnedNodeIdsViaDimensionalityAndTag(mesh, tag, 2);
}
std::vector<int> inf::extractAllNodeIdsIn2DCells(const inf::MeshInterface& mesh) {
    return extractAllNodeIdsInCellsWithDimensionality(mesh, 2);
}
std::vector<Parfait::Point<double>> inf::calcSurfaceAreaAtNodes(const inf::MeshInterface& mesh,
                                                                const std::set<int>& cell_ids) {
    std::vector<Parfait::Point<double>> area(mesh.nodeCount(), {0.0, 0.0, 0.0});
    for (int cell_id : cell_ids) {
        auto cell_type = mesh.cellType(cell_id);
        if (cell_type == inf::MeshInterface::TRI_3) {
            std::array<int, 3> cell_local_node_ids;
            std::array<Parfait::Point<double>, 3> cell_points;
            mesh.cell(cell_id, cell_local_node_ids.data());
            for (int corner = 0; corner < 3; ++corner)
                cell_points[corner] = mesh.node(cell_local_node_ids[corner]);
            auto corner_areas = Parfait::Metrics::calcCornerAreas(cell_points);
            for (int corner = 0; corner < 3; ++corner) {
                int node_id = cell_local_node_ids[corner];
                if (mesh.ownedNode(node_id)) area[node_id] += corner_areas[corner];
            }
        } else if (cell_type == inf::MeshInterface::QUAD_4) {
            std::array<int, 4> cell_local_node_ids;
            std::array<Parfait::Point<double>, 4> cell_points;
            mesh.cell(cell_id, cell_local_node_ids.data());
            for (int corner = 0; corner < 4; ++corner)
                cell_points[corner] = mesh.node(cell_local_node_ids[corner]);
            auto corner_areas = Parfait::Metrics::calcCornerAreas(cell_points);
            for (int corner = 0; corner < 4; ++corner) {
                int node_id = cell_local_node_ids[corner];
                if (mesh.ownedNode(node_id)) area[node_id] += corner_areas[corner];
            }
        }
    }
    return area;
}

std::vector<double> inf::extractOwnedDataAtNodes(const std::vector<double>& node_data,
                                                 const inf::MeshInterface& mesh) {
    std::vector<double> field;
    for (int n = 0; n < mesh.nodeCount(); n++) {
        if (mesh.ownedNode(n)) {
            field.push_back(node_data[n]);
        }
    }
    return field;
}
std::vector<double> inf::extractOwnedDataAtCells(const std::vector<double>& cell_data,
                                                 const inf::MeshInterface& mesh) {
    std::vector<double> field;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.ownedCell(c)) {
            field.push_back(cell_data[c]);
        }
    }
    return field;
}
