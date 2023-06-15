#include <algorithm>
#include "IsoSampling.h"
#include "PluginLocator.h"
#include "Shortcuts.h"
#include <parfait/Checkpoint.h>
void addCellToCut(inf::isosampling::Isosurface& cut,
                  const std::vector<std::array<int, 2>>& edges,
                  const std::vector<int>& cell_nodes,
                  const std::function<Parfait::Point<double>(int)>& get_point,
                  const std::function<double(int)>& get_iso_value);

long calcMyStart(MessagePasser mp, long my_count) {
    std::vector<long> all_counts = mp.Gather(my_count);
    long my_offset = 0;
    for (int r = 0; r < mp.Rank(); r++) {
        my_offset += all_counts[r];
    }
    return my_offset;
}
double computeChainLength(const std::vector<std::vector<double>>& p2p_dist,
                          const std::vector<int>& chain_order) {
    double dist = 0;
    int num_nodes = chain_order.size();
    for (int i = 0; i < num_nodes; i++) {
        int n1 = chain_order[i];
        int n2 = chain_order[(i + 1) % num_nodes];
        dist += p2p_dist[n1][n2];
    }
    return dist;
}

std::vector<std::array<int, 2>> getCellEdges(inf::MeshInterface::CellType type) {
    using Type = inf::MeshInterface::CellType;
    switch (type) {
        case Type::NODE: {
            return {};
        }
        case Type::BAR_2: {
            return std::vector<std::array<int, 2>>{{0, 1}};
        }
        case Type::TRI_3: {
            auto edges_array = Parfait::CGNS::Triangle::edge_to_node;
            return std::vector<std::array<int, 2>>(edges_array.begin(), edges_array.end());
        }
        case Type::QUAD_4: {
            auto edges_array = Parfait::CGNS::Quad::edge_to_node;
            return std::vector<std::array<int, 2>>(edges_array.begin(), edges_array.end());
        }
        case Type::TETRA_4: {
            auto edges_array = Parfait::CGNS::Tet::edge_to_node;
            return std::vector<std::array<int, 2>>(edges_array.begin(), edges_array.end());
        }
        case Type::PYRA_5: {
            auto edges_array = Parfait::CGNS::Pyramid::edge_to_node;
            return std::vector<std::array<int, 2>>(edges_array.begin(), edges_array.end());
        }
        case Type::PENTA_6: {
            auto edges_array = Parfait::CGNS::Prism::edge_to_node;
            return std::vector<std::array<int, 2>>(edges_array.begin(), edges_array.end());
        }
        case Type::HEXA_8: {
            auto edges_array = Parfait::CGNS::Hex::edge_to_node;
            return std::vector<std::array<int, 2>>(edges_array.begin(), edges_array.end());
        }
        default:
            PARFAIT_THROW("getCellEdges as vector not implemented for type " +
                          inf::MeshInterface::cellTypeString(type));
    }
}

int inf::isosampling::Isosurface::add(const CutEdge& edge) {
    auto p = std::make_pair(edge.a, edge.b);
    if (edge_number.count(p) == 0) {
        int number = int(edge_weights.size());
        edge_number[p] = number;
        edge_weights.push_back(edge.w);
        edge_nodes.push_back({edge.a, edge.b});
        return number;
    }
    return edge_number.at(p);
}
int inf::isosampling::Isosurface::addTri(const CutEdge& a, const CutEdge& b, const CutEdge& c) {
    std::array<int, 3> tri;
    tri[0] = add(a);
    tri[1] = add(b);
    tri[2] = add(c);
    tri_edges.push_back(tri);
    return (tri_edges.size()) - 1;
}
int inf::isosampling::Isosurface::addBar(const CutEdge& a, const CutEdge& b) {
    std::array<int, 2> bar;
    bar[0] = add(a);
    bar[1] = add(b);
    bar_edges.push_back(bar);
    return (bar_edges.size()) - 1;
}
std::shared_ptr<inf::TinfMesh> inf::isosampling::Isosurface::getMesh() const {
    auto data = inf::TinfMeshData();
    int num_nodes = int(edge_number.size());
    data.points.resize(num_nodes);
    for (int node = 0; node < num_nodes; node++) {
        auto& edge = edge_nodes.at(node);
        Parfait::Point<double> a, b;
        mesh->nodeCoordinate(edge[0], a.data());
        mesh->nodeCoordinate(edge[1], b.data());
        double w = edge_weights.at(node);
        data.points.at(node) = w * a + (1.0 - w) * b;
    }
    data.global_node_id.resize(size_t(num_nodes));
    long global_node_id_offset = calcMyStart(mp, num_nodes);
    for (unsigned int i = 0; i < data.global_node_id.size(); i++) {
        data.global_node_id[i] = global_node_id_offset + long(i);
    }
    data.node_owner.resize(size_t(num_nodes), mesh->partitionId());

    long global_cell_id_offset = 0;
    long num_bars = long(bar_edges.size());
    // create bars
    {
        auto& bars = data.cells[inf::MeshInterface::BAR_2];
        bars.resize(2 * size_t(num_bars));
        int index = 0;
        data.global_cell_id[inf::MeshInterface::BAR_2].resize(num_bars);
        long global_bar_offset = global_cell_id_offset + calcMyStart(mp, num_bars);
        for (int t = 0; t < num_bars; t++) {
            data.global_cell_id[inf::MeshInterface::BAR_2][t] = global_bar_offset + t;
            for (int i = 0; i < 2; i++) bars[index++] = bar_edges[t][i];
        }

        auto& tags = data.cell_tags[inf::MeshInterface::BAR_2];
        tags.resize(size_t(num_bars), 0);
        data.cell_owner[inf::MeshInterface::BAR_2].resize(num_bars, mesh->partitionId());
    }
    global_cell_id_offset += mp.ParallelSum(num_bars);

    // create triangles
    long num_tris = long(tri_edges.size());
    {
        auto& triangles = data.cells[inf::MeshInterface::TRI_3];
        triangles.resize(3 * size_t(num_tris));
        int index = 0;
        data.global_cell_id[inf::MeshInterface::TRI_3].resize(num_tris);
        long global_tri_offset = global_cell_id_offset + calcMyStart(mp, num_tris);
        for (int t = 0; t < num_tris; t++) {
            data.global_cell_id[inf::MeshInterface::TRI_3][t] = global_tri_offset + t;
            for (int i = 0; i < 3; i++) triangles[index++] = tri_edges[t][i];
        }

        auto& tags = data.cell_tags[inf::MeshInterface::TRI_3];
        tags.resize(size_t(num_tris), 0);
        data.cell_owner[inf::MeshInterface::TRI_3].resize(num_tris, mesh->partitionId());
    }
    global_cell_id_offset += mp.ParallelSum(num_tris);

    auto m = std::make_shared<inf::TinfMesh>(data, mesh->partitionId());
    return m;
}
std::shared_ptr<inf::FieldInterface> inf::isosampling::Isosurface::apply(
    const inf::FieldInterface& f) {
    PARFAIT_ASSERT(f.blockSize() == 1, "Can only sample scalar fields");
    PARFAIT_ASSERT(f.association() == FieldAttributes::Node(),
                   "Field: " + f.name() + " can only create an iso-surface of a node field");

    int num_nodes = int(edge_number.size());
    std::vector<double> sampled_field(num_nodes);
    for (int node = 0; node < num_nodes; node++) {
        auto& edge = edge_nodes[node];
        double a, b;
        f.value(edge[0], &a);
        f.value(edge[1], &b);
        double w = edge_weights[node];
        sampled_field[node] = w * a + (1.0 - w) * b;
    }

    auto out = std::make_shared<inf::VectorFieldAdapter>(
        f.name(), inf::FieldAttributes::Node(), sampled_field);
    auto attributes = f.getAllAttributes();
    for (auto& pair : attributes) {
        auto key = pair.first;
        auto value = pair.second;
        out->setAdapterAttribute(key, value);
    }
    return out;
}
inf::isosampling::Isosurface::Isosurface(MessagePasser mp_in,
                                         std::shared_ptr<inf::MeshInterface> mesh_in,
                                         std::function<double(int)> get_nodal_iso_value)
    : mp(mp_in), mesh(mesh_in) {
    auto get_point = [&](int node_id) {
        Parfait::Point<double> p = mesh->node(node_id);
        return p;
    };
    std::vector<int> cell_nodes;
    for (int c = 0; c < mesh->cellCount(); c++) {
        if (not mesh->ownedCell(c)) continue;
        mesh->cell(c, cell_nodes);
        auto edges = getCellEdges(mesh->cellType(c));
        addCellToCut(*this, edges, cell_nodes, get_point, get_nodal_iso_value);
    }
}
std::function<double(int)> inf::isosampling::sphere(const inf::MeshInterface& mesh,
                                                    Parfait::Point<double> center,
                                                    double radius) {
    auto get_node_value = [=, &mesh](int node_id) {
        Parfait::Point<double> p = mesh.node(node_id);
        return (p - center).magnitude() - radius;
    };
    return get_node_value;
}
std::function<double(int)> inf::isosampling::plane(const inf::MeshInterface& mesh,
                                                   Parfait::Point<double> center,
                                                   Parfait::Point<double> normal) {
    auto get_node_value = [=, &mesh](int node_id) {
        Parfait::Point<double> p = mesh.node(node_id);
        auto v = (p - center);
        auto d = Parfait::Point<double>::dot(v, normal) * normal;
        return Parfait::Point<double>::dot(d, normal);
    };
    return get_node_value;
}
std::function<double(int)> inf::isosampling::field_isosurface(const inf::FieldInterface& field,
                                                              double value) {
    auto get_node_value = [=, &field](int node_id) {
        double v;
        field.value(node_id, &v);
        return value - v;
    };
    return get_node_value;
}
void addCellToCut(inf::isosampling::Isosurface& cut,
                  const std::vector<std::array<int, 2>>& edges,
                  const std::vector<int>& cell_nodes,
                  const std::function<Parfait::Point<double>(int)>& get_point,
                  const std::function<double(int)>& get_iso_value) {
    std::vector<inf::isosampling::CutEdge> cut_edges;
    for (auto e : edges) {
        int n1 = cell_nodes[e[0]];
        int n2 = cell_nodes[e[1]];
        double a = get_iso_value(n1);
        double b = get_iso_value(n2);
        if ((a > 0.0 and b <= 0.0) or (b > 0.0 and a <= 0.0)) {
            double w = a / (a - b);
            cut_edges.push_back(inf::isosampling::CutEdge(n1, n2, 1.0 - w));
        }
    }

    if (cut_edges.size() == 0) return;
    if (cut_edges.size() == 2) {
        cut.addBar(cut_edges[0], cut_edges[1]);
        return;
    }
    if (cut_edges.size() == 3) {
        cut.addTri(cut_edges[0], cut_edges[1], cut_edges[2]);
        return;
    }

    // sort cut_edges using TSP solution
    int chain_length = cut_edges.size();
    std::vector<std::vector<double>> p2p_distance(chain_length, std::vector<double>(chain_length));
    for (int i = 0; i < chain_length; i++) {
        double w1 = cut_edges[i].w;
        auto point_a = w1 * get_point(cut_edges[i].a) + (1.0 - w1) * get_point(cut_edges[i].b);
        for (int j = 0; j < i; j++) {
            double w2 = cut_edges[j].w;
            auto point_b = w2 * get_point(cut_edges[j].a) + (1.0 - w2) * get_point(cut_edges[j].b);
            p2p_distance[i][j] = (point_b - point_a).magnitude();
            p2p_distance[j][i] = p2p_distance[i][j];
        }
    }
    std::vector<int> edge_order;
    for (int i = 0; i < int(cut_edges.size()); i++) {
        edge_order.push_back(i);
    }

    std::vector<int> best_order = edge_order;
    double min_dist = computeChainLength(p2p_distance, edge_order);
    do {
        double current_dist = computeChainLength(p2p_distance, edge_order);
        if (current_dist < min_dist) {
            best_order = edge_order;
            min_dist = current_dist;
        }
    } while (std::next_permutation(edge_order.begin() + 1, edge_order.end()));

    for (int i = 1; i < chain_length - 1; i++) {
        auto edge_a = cut_edges[best_order[0]];
        auto edge_b = cut_edges[best_order[i]];
        auto edge_c = cut_edges[best_order[i + 1]];
        cut.addTri(edge_a, edge_b, edge_c);
    }
}

inf::isosampling::CutEdge::CutEdge(int first, int second, double weight) {
    if (first < second) {
        a = first;
        b = second;
        w = weight;
    } else {
        b = first;
        a = second;
        w = 1.0 - weight;
    }
}
