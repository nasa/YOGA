#include <t-infinity/MeshMechanics.h>

using namespace inf;

double Mechanics::calcCavityCost(
    const std::vector<inf::experimental::CellTypeAndNodeIds>& cells_in_cavity,
    const TinfMesh& mesh) {
    double max_cost = 0.0;
    std::vector<Parfait::Point<double>> cell_vertices;
    for (auto& c : cells_in_cavity) {
        cell_vertices.clear();
        for (int node_id : c.node_ids) cell_vertices.push_back(mesh.node(node_id));
        double cost = inf::MeshQuality::cellHilbertCost(c.type, cell_vertices);
        max_cost = std::max(cost, max_cost);
    }
    return max_cost;
}

int Mechanics::getIdOfBestSteinerNodeForCavity(experimental::MikeCavity& cavity,
                                               experimental::MeshBuilder& builder) {
    std::set<int> candidate_steiner_nodes;
    for (auto& f : cavity.exposed_faces)
        for (int id : f) candidate_steiner_nodes.insert(id);
    std::vector<inf::Cell> cavity_cells;
    int best_steiner_id = 0;
    double lowest_cost = std::numeric_limits<double>::max();
    //    printf("Checking cavity for potential swaps\n");
    for (int candidate : candidate_steiner_nodes) {
        auto cells_in_cavity = builder.generateCellsToAdd(cavity, candidate);
        double cost = calcCavityCost(cells_in_cavity, *builder.mesh);
        //        printf(" candidate node id %d, cost %e\n", candidate, cost);
        if (cost < lowest_cost) {
            best_steiner_id = candidate;
            lowest_cost = cost;
        }
    }
    //    printf("using steiner node %d\n", best_steiner_id);
    //    printf("\n");
    return best_steiner_id;
}

std::pair<double, double> Mechanics::calcMinAndMaxEdgeLength(
    const TinfMesh& mesh,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
    int dimension) {
    double min_edge_length = std::numeric_limits<double>::max();
    double max_edge_length = 0.0;
    for (auto& pair : mesh.mesh.cells) {
        auto type = pair.first;
        auto& cells = pair.second;
        if (dimension == inf::MeshInterface::cellTypeDimensionality(type)) {
            int cell_length = inf::MeshInterface::cellTypeLength(type);
            int num_cells_in_type = cells.size() / cell_length;
            for (int index = 0; index < num_cells_in_type; index++) {
                if (cells[index * cell_length] < 0) continue;  // skip lazy deleted cells
                inf::Cell cell(mesh, type, index, dimension);
                for (auto& edge : cell.edges()) {
                    auto a = Parfait::Point<double>{mesh.node(edge.front())};
                    auto b = Parfait::Point<double>{mesh.node(edge.back())};
                    double edge_length_in_metric_space = calc_edge_length(a, b);
                    max_edge_length = std::max(max_edge_length, edge_length_in_metric_space);
                    min_edge_length = std::min(min_edge_length, edge_length_in_metric_space);
                }
            }
        }
    }
    return {min_edge_length, max_edge_length};
}

bool Mechanics::edgesMatch(const std::array<int, 2>& a, const std::array<int, 2>& b) {
    int n = std::count(a.begin(), a.end(), b.front());
    n += std::count(a.begin(), a.end(), b.back());
    return 2 == n;
}

std::vector<inf::experimental::CellEntry> Mechanics::getCellsContainingEdge(
    const TinfMesh& mesh, const std::array<int, 2>& edge, int dimension) {
    std::vector<inf::experimental::CellEntry> entries;

    for (auto& cells : mesh.mesh.cells) {
        auto cell_type = cells.first;
        auto cell_length = mesh.cellTypeLength(cell_type);
        if (isCellSameDimensionOrOneLess(mesh, dimension, cell_type)) {
            auto& nodes_in_cells = cells.second;
            int ncells_in_type = nodes_in_cells.size() / cell_length;
            for (int id_in_type = 0; id_in_type < ncells_in_type; id_in_type++) {
                auto first_node_in_cell = nodes_in_cells[id_in_type * cell_length];
                if (first_node_in_cell < 0) continue;
                inf::Cell cell(mesh, cell_type, id_in_type);
                for (auto& e : cell.edges()) {
                    if (edgesMatch(edge, e)) entries.push_back({cell_type, id_in_type});
                }
            }
        }
    }
    return entries;
}

bool Mechanics::areAnyOfTheseCellsOnTheBoundary(
    int dimension, const std::vector<inf::experimental::CellEntry>& cell_list) {
    using T = inf::MeshInterface::CellType;
    std::vector<T> boundary_types_2d = {T::BAR_2, T::NODE};
    std::vector<T> boundary_types_3d = {T::TRI_3, T::QUAD_4};
    for (auto& cell : cell_list) {
        if (Parfait::VectorTools::isIn(boundary_types_2d, cell.type)) return true;
        if (dimension == 3) {
            if (Parfait::VectorTools::isIn(boundary_types_3d, cell.type)) return true;
        }
    }
    return false;
}

void Mechanics::copyBuilderAndSyncAndVisualize(
    inf::experimental::MeshBuilder builder_copy,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length) {
    Tracer::begin("copy-and-viz");
    builder_copy.sync();
    std::vector<double> longest_edge_length_in_cell(builder_copy.mesh->cellCount(), 0.0);
    std::vector<double> shortest_edge_length_in_cell(builder_copy.mesh->cellCount(), 1000);
    for (int cell_id = 0; cell_id < builder_copy.mesh->cellCount(); cell_id++) {
        inf::Cell cell(builder_copy.mesh, cell_id);
        for (auto edge : cell.edges()) {
            Parfait::Point<double> a = builder_copy.mesh->node(edge.front());
            Parfait::Point<double> b = builder_copy.mesh->node(edge.back());
            auto length = calc_edge_length(a, b);
            longest_edge_length_in_cell[cell_id] =
                std::max(length, longest_edge_length_in_cell[cell_id]);
            shortest_edge_length_in_cell[cell_id] =
                std::min(length, shortest_edge_length_in_cell[cell_id]);
        }
        if (cell.edges().size() == 0) {
            shortest_edge_length_in_cell[cell_id] = 1.0;
            longest_edge_length_in_cell[cell_id] = 1.0;
        }
    }
    auto longest = std::make_shared<inf::VectorFieldAdapter>(
        "longest_edge_length", inf::FieldAttributes::Cell(), longest_edge_length_in_cell);
    auto shortest = std::make_shared<inf::VectorFieldAdapter>(
        "shortest_edge_length", inf::FieldAttributes::Cell(), shortest_edge_length_in_cell);
    inf::shortcut::visualize(
        viz_name + "." + std::to_string(viz_step++), mp, builder_copy.mesh, {longest, shortest});
    Tracer::end("copy-and-viz");
}

std::vector<inf::experimental::CellEntry> Mechanics::identifyTopPercentOfPoorQualityCells(
    const experimental::MeshBuilder& builder, double percent) {
    std::map<inf::MeshInterface::CellType, std::vector<double>> cells_cost;
    std::priority_queue<std::tuple<double, inf::experimental::CellEntry>> queue;
    for (auto& pair : builder.mesh->mesh.cells) {
        auto type = pair.first;
        if (type == inf::MeshInterface::NODE or type == inf::MeshInterface::BAR_2) continue;
        auto& cells = pair.second;
        int cell_length = inf::MeshInterface::cellTypeLength(type);
        int num_cells_in_type = cells.size() / cell_length;
        for (int index = 0; index < num_cells_in_type; index++) {
            inf::Cell cell(*builder.mesh, type, index, builder.dimension);
            double cost = inf::MeshQuality::cellHilbertCost(cell.type(), cell.points());
            queue.push({cost, {type, index}});
        }
    }
    int max_cells_to_return = std::max(1.0, percent * builder.mesh->cellCount() / 100.);
    std::vector<inf::experimental::CellEntry> worst_cells;
    max_cells_to_return = std::min(max_cells_to_return, int(queue.size()));
    for (int i = 0; i < max_cells_to_return; i++) {
        inf::experimental::CellEntry entry = std::get<1>(queue.top());
        queue.pop();
        worst_cells.push_back(entry);
    }
    return worst_cells;
}

void Mechanics::swapAllEdges(
    experimental::MeshBuilder& builder,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length) {
    Tracer::begin("swap edges");
    auto cells_with_poor_quality = identifyTopPercentOfPoorQualityCells(builder, 100.0);
    printf("Starting edge swap pass. %d\n", int(cells_with_poor_quality.size()));
    int num_edge_swaps = 0;
    int max_passes = 3;
    int pass = 0;
    do {
        num_edge_swaps = 0;
        for (auto cell_entry : cells_with_poor_quality) {
            if (builder.isCellNull(cell_entry)) continue;
            if (builder.hasCellBeenModified(cell_entry)) {
                continue;
            }
            inf::Cell cell(*builder.mesh, cell_entry.type, cell_entry.index);
            if (cell.edges().size() < 2) {
                continue;
            }
            for (auto edge : cell.edges()) {
                if (swapEdge(builder, edge)) {
                    num_edge_swaps++;
                }
            }
        }
        printf("Swapped %d edges\n", num_edge_swaps);
        builder.sync();  // to reset cell has been modified
    } while (num_edge_swaps > 0 and max_passes > pass++);
    Tracer::end("swap edges");
}

void Mechanics::collapseAllEdges(
    experimental::MeshBuilder& builder,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
    double edge_collapse_threshold) {
    Tracer::begin("collapse edges");
    int num_edge_collapse = 0;
    auto edges = getShortestEdges(builder, calc_edge_length, 10.0);
    do {
        num_edge_collapse = 0;
        for (auto edge : edges) {
            Parfait::Point<double> a = builder.mesh->node(edge[0]);
            Parfait::Point<double> b = builder.mesh->node(edge[1]);

            auto metric_length = calc_edge_length(a, b);
            if (metric_length < edge_collapse_threshold) {
                if (collapseEdge(builder, edge, calc_edge_length)) num_edge_collapse++;
            }
        }
        printf("Collapsed %d edges\n", num_edge_collapse);
        builder.sync();  // to reset cell has been modified
        break;
    } while (num_edge_collapse > 0);
    Tracer::end("collapse edges");
}

int Mechanics::splitAllEdges(
    experimental::MeshBuilder& builder,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
    double edge_split_threshold) {
    Tracer::begin("split edges");
    printf("\nStarting edge split pass\n");
    int num_edge_splits = 0;
    int pass = 0;
    int max_passes = 3;
    int total_edge_splits = 0;
    do {
        num_edge_splits = 0;
        auto longest_edges = getLongestEdges(builder, calc_edge_length, 100.0);
        for (auto edge : longest_edges) {
            Parfait::Point<double> a = builder.mesh->node(edge[0]);
            Parfait::Point<double> b = builder.mesh->node(edge[1]);
            if (calc_edge_length(a, b) > edge_split_threshold) {
                splitEdge(builder, edge, calc_edge_length);
                num_edge_splits++;
            }
            //            else {
            //                break;
            //            }
        }
        total_edge_splits += num_edge_splits;
        printf(" pass %d/%d: split %d edges\n", pass, max_passes, num_edge_splits);
        builder.sync();  // to reset cell has been modified
        copyBuilderAndSyncAndVisualize(builder, calc_edge_length);
        pass++;
    } while (num_edge_splits > 0 and pass < max_passes);
    Tracer::end("split edges");
    return total_edge_splits;
}

bool Mechanics::swapEdge(experimental::MeshBuilder& builder, std::array<int, 2> edge) {
    auto cells_containing_edge = getCellsContainingEdge(*builder.mesh, edge, builder.dimension);
    bool is_edge_on_boundary = isEdgeOnBoundary(*builder.mesh, edge, builder.dimension);
    if (is_edge_on_boundary) {
        return false;
    }

    inf::experimental::MikeCavity cavity(builder.dimension);
    for (auto& cell_entry : cells_containing_edge) {
        cavity.addCell(*builder.mesh, cell_entry);
    }
    cavity.cleanup();
    auto node_id = getIdOfBestSteinerNodeForCavity(cavity, builder);
    builder.replaceCavity(cavity, node_id);
    if (node_id == edge[0] or node_id == edge[1]) {
        return false;  // we reconnected the cavity to be the same
    } else {
        return true;  // we actually did a swap
    }
}

void Mechanics::splitEdge(
    experimental::MeshBuilder& builder,
    std::array<int, 2> edge,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length) {
    auto cells_containing_edge = getCellsContainingEdge(*builder.mesh, edge, builder.dimension);
    bool is_edge_on_boundary = isEdgeOnBoundary(*builder.mesh, edge, builder.dimension);

    inf::experimental::MikeCavity cavity(builder.dimension);
    auto a = Parfait::Point<double>(builder.mesh->node(edge[0]));
    auto b = Parfait::Point<double>(builder.mesh->node(edge[1]));
    double edge_length = calc_edge_length(a, b);
    if (edge_length < 1.1) {
        printf("All edges within tolerance\n");
        return;
    }

    // edge split
    double alpha = 0.5;  // 1.0 / edge_length;

    Parfait::Point<double> mid_edge_point = alpha * a + (1.0 - alpha) * b;
    int new_node_id = builder.addNode(mid_edge_point);
    if (is_edge_on_boundary) {
        auto geom_association = builder.calcEdgeGeomAssociation(edge[0], edge[1]);
        builder.node_geom_association[new_node_id].push_back(geom_association);
        if (builder.db != nullptr) {
            if (builder.dimension == 3) PARFAIT_THROW("Not implemented for 3D")
            auto db_edge = builder.db->getEdge(geom_association.index);
            mid_edge_point = db_edge->project(mid_edge_point);
            builder.mesh->mesh.points[new_node_id] = mid_edge_point;
        }
    }

    for (auto& cell_entry : cells_containing_edge) {
        cavity.addCell(*builder.mesh, cell_entry);
    }

    //    if (is_edge_on_boundary) {
    //        builder.growCavityAroundPoint_boundary(new_node_id, calc_edge_length, cavity);
    //    } else {
    //        builder.growCavityAroundPoint_interior(new_node_id, calc_edge_length, cavity);
    //    }
    cavity.cleanup();
    // cavity.visualizePoints(*builder.mesh, "cavity-points");
    builder.replaceCavity(cavity, new_node_id);
}

bool Mechanics::collapseEdge(
    experimental::MeshBuilder& builder,
    std::array<int, 2> edge,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length) {
    int node_a = edge[0];
    int node_b = edge[1];
    bool is_node_a_critical = isNodeCriticalOnGeometry(builder, node_a);
    bool is_node_b_critical = isNodeCriticalOnGeometry(builder, node_b);
    bool is_node_a_on_boundary = builder.isNodeOnBoundary(node_a);
    bool is_node_b_on_boundary = builder.isNodeOnBoundary(node_b);

    if (is_node_a_critical and is_node_b_critical) return false;
    if (is_node_a_on_boundary and is_node_b_on_boundary) {
        PARFAIT_WARNING(
            "Trying to collapse edge on boundary.  This could be allowed if edge spans only one "
            "geom entity, but we "
            "don't have logic yet");
        return false;
    }

    int node_to_delete = node_a;
    int node_to_keep = node_b;
    if (is_node_a_on_boundary) {
        node_to_delete = node_b;
        node_to_keep = node_a;
    } else if (is_node_b_on_boundary) {
        node_to_delete = node_a;
        node_to_keep = node_b;
    }
    auto cells_touching_deleted_node = builder.node_to_cells[node_to_delete];
    inf::experimental::MikeCavity cavity(builder.dimension);
    for (auto& cell_entry : cells_touching_deleted_node) cavity.addCell(*builder.mesh, cell_entry);
    cavity.cleanup();
    builder.replaceCavity(cavity, node_to_keep);
    return true;
}

int Mechanics::subdivideCell(experimental::MeshBuilder& builder,
                             inf::experimental::CellEntry cell_entry,
                             Parfait::Point<double> p) {
    int new_node_id = builder.addNode(p);

    inf::experimental::MikeCavity cavity(builder.dimension);
    cavity.addCell(*builder.mesh, cell_entry);
    cavity.cleanup();
    builder.replaceCavity(cavity, new_node_id);
    return new_node_id;
}

std::vector<inf::experimental::CellEntry>
Mechanics::identifyCellsWithLongestOrShortestEdgesInMetric(
    const TinfMesh& mesh,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
    double percent,
    int dimension,
    bool invert) {
    using CellTuple = std::tuple<double, inf::experimental::CellEntry>;
    auto compare = [=](CellTuple a, CellTuple b) {
        bool isLess = std::get<0>(a) < std::get<0>(b);
        if (not invert) return isLess;
        return !isLess;
    };
    std::priority_queue<CellTuple, std::vector<CellTuple>, decltype(compare)> queue(compare);
    for (auto& pair : mesh.mesh.cells) {
        auto type = pair.first;
        auto& cells = pair.second;
        if (dimension == inf::MeshInterface::cellTypeDimensionality(type)) {
            int cell_length = inf::MeshInterface::cellTypeLength(type);
            int num_cells_in_type = cells.size() / cell_length;
            for (int index = 0; index < num_cells_in_type; index++) {
                if (cells[cell_length * index] == -1) continue;  // skip lazy deleted cells
                inf::Cell cell(mesh, type, index, dimension);
                double longest_edge_length = 0.0;
                for (auto& edge : cell.edges()) {
                    auto a = Parfait::Point<double>{mesh.node(edge.front())};
                    auto b = Parfait::Point<double>{mesh.node(edge.back())};
                    double edge_length_in_metric_space = calc_edge_length(a, b);
                    if (edge_length_in_metric_space > longest_edge_length) {
                        longest_edge_length = edge_length_in_metric_space;
                    }
                }
                queue.push({longest_edge_length, {type, index}});
            }
        }
    }

    std::vector<inf::experimental::CellEntry> worst_cells;
    int max_cells_to_return = std::max(1.0, percent * mesh.cellCount() / 100.);
    max_cells_to_return = std::min(max_cells_to_return, int(queue.size()));
    for (int i = 0; i < max_cells_to_return; i++) {
        inf::experimental::CellEntry entry = std::get<1>(queue.top());
        queue.pop();
        worst_cells.push_back(entry);
    }
    return worst_cells;
}

int Mechanics::insertNode(experimental::MeshBuilder& builder, Parfait::Point<double> p) {
    int node_id = builder.addNode(p);
    inf::experimental::MikeCavity cavity(builder.dimension);
    auto cells_touching_node = getCellsOverlappingNode(node_id, builder);
    for (auto& cell : cells_touching_node) cavity.addCell(*builder.mesh, cell);
    builder.replaceCavity(cavity, node_id);
    builder.sync();
    return node_id;
}

std::vector<inf::experimental::CellEntry> Mechanics::getCellsOverlappingNode(
    int node_id, experimental::MeshBuilder& builder) {
    std::vector<inf::experimental::CellEntry> overlapping_cells;
    Parfait::Point<double> p = builder.mesh->node(node_id);

    if (2 == builder.dimension) {
        int n = builder.mesh->cellCount(MeshInterface::TRI_3);
        for (int i = 0; i < n; i++) {
            Cell cell(*builder.mesh, MeshInterface::TRI_3, i, builder.dimension);
            if (cell.contains(p)) overlapping_cells.push_back({MeshInterface::TRI_3, i});
        }
        n = builder.mesh->cellCount(MeshInterface::QUAD_4);
        for (int i = 0; i < n; i++) {
            Cell cell(*builder.mesh, MeshInterface::QUAD_4, i, builder.dimension);
            if (cell.contains(p)) overlapping_cells.push_back({MeshInterface::QUAD_4, i});
        }
    } else {
        PARFAIT_THROW("Get overlapping cells not implemented for 3D.")
    }
    return overlapping_cells;
}
bool Mechanics::combineNeighborsToQuad(experimental::MeshBuilder& builder,
                                       std::array<int, 2> edge,
                                       double cost_threshold) {
    if (isEdgeOnBoundary(*builder.mesh, edge, builder.dimension)) return false;
    auto cells_containing_edge = getCellsContainingEdge(*builder.mesh, edge, builder.dimension);

    bool only_has_triangle_neighbors = true;
    for (auto& cell_entry : cells_containing_edge) {
        if (inf::MeshInterface::TRI_3 != cell_entry.type) {
            only_has_triangle_neighbors = false;
            break;
        }
    }
    if (not only_has_triangle_neighbors) return false;

    inf::experimental::MikeCavity cavity(builder.dimension);
    for (auto& cell_entry : cells_containing_edge) {
        cavity.addCell(*builder.mesh, cell_entry);
    }
    cavity.cleanup();
    auto node_id = getIdOfBestSteinerNodeForCavity(cavity, builder);
    return builder.replaceCavityTryQuads(cavity, node_id, cost_threshold);
}
void Mechanics::tryMakingQuads(
    experimental::MeshBuilder& builder,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length) {
    Tracer::begin("make quads");
    int num_created_quads = 0;
    std::vector<double> cost_thresholds = {0.02, 0.6, 0.7};

    for (auto cost_threshold : cost_thresholds) {
        do {
            num_created_quads = 0;
            for (auto& pair : builder.mesh->mesh.cell_tags) {
                auto type = pair.first;
                int num_cells = pair.second.size();
                for (int cell_index = 0; cell_index < num_cells; cell_index++) {
                    auto cell_entry = inf::experimental::CellEntry{type, cell_index};
                    if (builder.hasCellBeenModified(cell_entry)) {
                        continue;
                    }
                    inf::Cell cell(*builder.mesh, cell_entry.type, cell_entry.index);
                    if (cell.edges().size() < 2) {
                        continue;
                    }
                    for (auto edge : cell.edges()) {
                        if (combineNeighborsToQuad(builder, edge, cost_threshold)) {
                            num_created_quads++;
                        }
                    }
                }
            }
            printf("Created %d quads below cost %1.2lf\n", num_created_quads, cost_threshold);
            copyBuilderAndSyncAndVisualize(builder, calc_edge_length);
            builder.sync();
        } while (num_created_quads > 0);
        Tracer::end("make quads");
    }
}
std::vector<std::array<int, 2>> Mechanics::sortEdges(
    inf::experimental::MeshBuilder& builder,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
    double percent,
    bool long_to_short) {
    std::set<std::array<int, 2>> already_sorted_edges;
    using EdgeTuple = std::tuple<double, int, int>;

    auto compare = [=](EdgeTuple a, EdgeTuple b) {
        bool isLess = std::get<0>(a) < std::get<0>(b);
        if (long_to_short) return isLess;
        return !isLess;
    };
    std::priority_queue<EdgeTuple, std::vector<EdgeTuple>, decltype(compare)> queue(compare);

    for (auto& pair : builder.mesh->mesh.cells) {
        auto type = pair.first;
        if (type == inf::MeshInterface::NODE or type == inf::MeshInterface::BAR_2) continue;
        auto& cells = pair.second;
        int cell_length = inf::MeshInterface::cellTypeLength(type);
        int num_cells_in_type = cells.size() / cell_length;
        for (int index = 0; index < num_cells_in_type; index++) {
            if (builder.isCellNull({type, index})) continue;
            inf::Cell cell(*builder.mesh, type, index, builder.dimension);
            auto edges = cell.edges();
            for (auto& e : edges) {
                if (e[1] > e[0]) std::swap(e[0], e[1]);
                if (already_sorted_edges.count(e)) continue;
                already_sorted_edges.insert({e[0], e[1]});
                double metric_length =
                    calc_edge_length(builder.mesh->node(e[0]), builder.mesh->node(e[1]));
                queue.push({metric_length, e[0], e[1]});
            }
        }
    }
    long num_edges = already_sorted_edges.size();
    long max_edges_to_return = std::max(1.0, percent * num_edges / 100.);
    std::vector<std::array<int, 2>> out;
    for (int i = 0; i < max_edges_to_return; i++) {
        auto node_a = std::get<1>(queue.top());
        auto node_b = std::get<2>(queue.top());
        queue.pop();
        out.push_back({node_a, node_b});
    }
    return out;
}
std::vector<std::array<int, 2>> Mechanics::getLongestEdges(
    inf::experimental::MeshBuilder& builder,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
    double percent) {
    return sortEdges(builder, calc_edge_length, percent, true);
}
std::vector<std::array<int, 2>> Mechanics::getShortestEdges(
    inf::experimental::MeshBuilder& builder,
    std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
    double percent) {
    return sortEdges(builder, calc_edge_length, percent, false);
}
