#include "SurfaceMesh.h"
#include "SurfaceEdgeNeighbors.h"
#include <parfait/TaubinSmoothing.h>
#include "Shortcuts.h"

bool doesCellHaveNeighbor(const std::vector<std::vector<int>>& all_cell_neighbors,
                          int current_cell,
                          int potentially_neighbor_cell) {
    PARFAIT_ASSERT_BOUNDS(all_cell_neighbors, current_cell, "Cell not in all_cell_neighbors");
    for (auto neighbor : all_cell_neighbors[current_cell]) {
        if (neighbor == potentially_neighbor_cell) return true;
    }
    return false;
}

bool doesCellHaveExactEdge(const std::vector<int>& cell_nodes, int node_1, int node_2) {
    int length = cell_nodes.size();
    for (int i = 0; i < length; i++) {
        int a = cell_nodes[i];
        int b = cell_nodes[(i + 1) % length];
        if (a == node_1 and b == node_2) return true;
    }
    return false;
}

std::vector<std::array<int, 2>> inf::SurfaceMesh::selectExposedEdges(
    const inf::MeshInterface& mesh, const std::vector<std::vector<int>>& edge_neighbors) {
    std::vector<std::array<int, 2>> non_manifold_edges;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.is2DCell(c)) {
            auto num_edges = inf::SurfaceEdgeNeighbors::numberOfEdges(mesh.cellType(c));
            for (int e = 0; e < num_edges; e++) {
                if (edge_neighbors[c][e] < 0) {
                    std::vector<int> cell_nodes;
                    mesh.cell(c, cell_nodes);
                    non_manifold_edges.push_back({cell_nodes[e], cell_nodes[(e + 1) % num_edges]});
                }
            }
        }
    }
    return non_manifold_edges;
}
class LoopFinder {
    typedef std::array<int, 2> Edge;

  public:
    LoopFinder(std::vector<Edge> edges) : edges(edges) {
        for (unsigned long index = 0; index < edges.size(); index++) {
            auto e = edges[index];
            node_to_edge[e[0]].insert(index);
            node_to_edge[e[1]].insert(index);
        }

        int num_edges = edges.size();
        for (int edge = 0; edge < num_edges; edge++) {
            unassigned_edges.insert(edge);
        }
    }

    std::vector<std::vector<int>> findAllLoops() {
        std::vector<std::vector<int>> loops;

        while (not unassigned_edges.empty()) {
            auto seed_edge = *unassigned_edges.begin();
            unassigned_edges.erase(seed_edge);

            std::vector<int> loop = findLoop(edges[seed_edge]);
            if (loop.size() >= 3) {
                loops.push_back(loop);
            }
        }
        return loops;
    }

    std::vector<int> findLoop(std::array<int, 2> first_edge) {
        int first_node = first_edge[0];
        int next_node = first_edge[1];

        std::vector<int> loop;
        loop.push_back(first_node);
        while (searchForNextNodeInLoop(next_node, loop)) {
        }
        if (next_node == first_node) {
            return loop;
        }
        return {};
    }
    bool searchForNextNodeInLoop(int& next_node, std::vector<int>& loop) {
        for (int edge : unassigned_edges) {
            if (inf::SurfaceEdgeNeighbors::edgeHasNode(edges[edge], next_node)) {
                loop.push_back(next_node);
                next_node = inf::SurfaceEdgeNeighbors::getOtherNode(edges[edge], next_node);
                unassigned_edges.erase(edge);
                return true;
            }
        }
        return false;
    }

  private:
    std::vector<Edge> edges;
    std::map<int, std::set<int>> node_to_edge;
    std::set<int> unassigned_edges;
};

std::vector<std::vector<int>> inf::SurfaceMesh::identifyLoops(
    std::vector<std::array<int, 2>> edges) {
    LoopFinder loop_finder(edges);
    return loop_finder.findAllLoops();
}
long findHighestGlobalCellId(const inf::MeshInterface& mesh) {
    long next_gid = -1;
    for (int c = 0; c < mesh.cellCount(); c++) {
        next_gid = std::max(mesh.globalCellId(c), next_gid);
    }
    return next_gid;
}

long findHighestGlobalNodeId(const inf::MeshInterface& mesh) {
    long next_gid = -1;
    for (int c = 0; c < mesh.nodeCount(); c++) {
        next_gid = std::max(mesh.globalNodeId(c), next_gid);
    }
    return next_gid;
}

std::shared_ptr<inf::TinfMesh> inf::SurfaceMesh::fillHoles(
    const inf::MeshInterface& mesh_in, const std::vector<std::vector<int>>& loops) {
    auto mesh = std::make_shared<inf::TinfMesh>(mesh_in);

    auto next_global_cell_id = findHighestGlobalCellId(mesh_in) + 1;
    auto next_global_node_id = findHighestGlobalNodeId(mesh_in) + 1;

    for (auto& loop : loops) {
        if (loop.size() == 3) {
            auto type = inf::MeshInterface::TRI_3;
            int a = loop[0];
            int b = loop[1];
            int c = loop[2];
            mesh->mesh.cells[type].push_back(a);
            mesh->mesh.cells[type].push_back(b);
            mesh->mesh.cells[type].push_back(c);
            mesh->mesh.cell_owner[type].push_back(0);
            mesh->mesh.cell_tags[type].push_back(-1);
            mesh->mesh.global_cell_id[type].push_back(next_global_cell_id);
        } else if (loop.size() == 4) {
            auto type = inf::MeshInterface::QUAD_4;
            int a = loop[0];
            int b = loop[1];
            int c = loop[2];
            int d = loop[3];
            mesh->mesh.cells[type].push_back(a);
            mesh->mesh.cells[type].push_back(b);
            mesh->mesh.cells[type].push_back(c);
            mesh->mesh.cells[type].push_back(d);
            mesh->mesh.cell_owner[type].push_back(0);
            mesh->mesh.cell_tags[type].push_back(-1);
            mesh->mesh.global_cell_id[type].push_back(next_global_cell_id);
        } else {
            int num_points = loop.size();
            for (int i = 0; i < num_points; i++) {
                int a = loop[i];
                int b = loop[(i + 1) % num_points];
                int c = mesh->mesh.points.size();
                auto type = inf::MeshInterface::TRI_3;
                mesh->mesh.cells[type].push_back(a);
                mesh->mesh.cells[type].push_back(b);
                mesh->mesh.cells[type].push_back(c);
                mesh->mesh.cell_owner[type].push_back(0);
                mesh->mesh.cell_tags[type].push_back(-1);
                mesh->mesh.global_cell_id[type].push_back(next_global_cell_id);
            }
            Parfait::Point<double> centroid{0, 0, 0};
            for (int n : loop) {
                Parfait::Point<double> p;
                mesh->nodeCoordinate(n, p.data());
                centroid += p;
            }
            centroid /= (double)num_points;
            mesh->mesh.points.push_back(centroid);
            mesh->mesh.global_node_id.push_back(next_global_node_id++);
        }
    }
    mesh->rebuild();
    return mesh;
}
std::shared_ptr<inf::TinfMesh> inf::SurfaceMesh::fillHoles(const inf::MeshInterface& mesh_in) {
    auto edge_neighbors = inf::SurfaceEdgeNeighbors::buildSurfaceEdgeNeighbors(mesh_in);
    std::vector<std::array<int, 2>> non_manifold_edges =
        inf::SurfaceMesh::selectExposedEdges(mesh_in, edge_neighbors);
    std::vector<std::vector<int>> loops = inf::SurfaceMesh::identifyLoops(non_manifold_edges);
    auto mesh = inf::SurfaceMesh::fillHoles(mesh_in, loops);
    return mesh;
}
bool inf::SurfaceMesh::isManifold(const inf::MeshInterface& mesh,
                                  const std::vector<std::vector<int>>& surface_edge_neighbors,
                                  std::vector<int>* f) {
    bool all_manifold = true;
    if (f != nullptr) {
        for (int n = 0; n < mesh.nodeCount(); n++) {
            (*f)[n] = 1;
        }
    }
    std::vector<int> cell;
    PARFAIT_ASSERT(int(surface_edge_neighbors.size()) == mesh.cellCount(),
                   "Unexpected surface-edge-neighbor length");
    for (int c = 0; c < mesh.cellCount(); c++) {
        int num_neighbors = surface_edge_neighbors[c].size();
        mesh.cell(c, cell);
        int cell_length = mesh.cellLength(c);
        PARFAIT_ASSERT(cell_length == num_neighbors, "edge-neighbors length unexpected");
        for (int i = 0; i < num_neighbors; i++) {
            int edge_node_1 = cell[i];
            int edge_node_2 = cell[(i + 1) % cell_length];
            int neighbor = surface_edge_neighbors[c][i];
            if (neighbor < 0 or neighbor >= mesh.cellCount()) {
                all_manifold = false;
                if (f != nullptr) {
                    (*f)[edge_node_1] = 0;
                    (*f)[edge_node_2] = 0;
                }
                break;
            }
            bool is_manifold = doesCellHaveNeighbor(surface_edge_neighbors, neighbor, c);
            if (not is_manifold) {
                all_manifold = false;
                if (f != nullptr) {
                    (*f)[edge_node_1] = 0;
                    (*f)[edge_node_2] = 0;
                }
            }
        }
    }
    return all_manifold;
}
bool inf::SurfaceMesh::isOriented(const inf::MeshInterface& mesh,
                                  const std::vector<std::vector<int>>& surface_edge_neighbors) {
    //--- check for manifold to know if all neighbors exist
    if (not isManifold(mesh, surface_edge_neighbors)) return false;

    std::vector<int> cell;
    std::vector<int> neighbor;
    for (int c = 0; c < mesh.cellCount(); c++) {
        int num_neighbors = surface_edge_neighbors[c].size();
        mesh.cell(c, cell);
        int cell_length = mesh.cellLength(c);
        for (int i = 0; i < num_neighbors; i++) {
            int edge_node_1 = cell[i];
            int edge_node_2 = cell[(i + 1) % cell_length];
            int neighbor_index = surface_edge_neighbors[c][i];
            mesh.cell(neighbor_index, neighbor);
            bool is_oriented = doesCellHaveExactEdge(neighbor, edge_node_2, edge_node_1);
            if (not is_oriented) {
                return false;
            }
        }
    }
    return true;
}
void inf::SurfaceMesh::taubinSmooth(inf::TinfMesh& mesh,
                                    const std::vector<std::vector<int>>& surface_node_neighbors,
                                    bool plot) {
    int num_points = mesh.mesh.points.size();
    printf("Tabin smoothing surface mesh with %d nodes\n", num_points);
    Parfait::Point<double> zero{0, 0, 0};
    auto saved_points = mesh.mesh.points;
    auto& points = mesh.mesh.points;
    auto mp = MessagePasser(MPI_COMM_SELF);
    for (int i = 0; i < 100; i++) {
        Parfait::TaubinSmoothing::taubinSmoothing(zero, surface_node_neighbors, points, 1);
        if (plot) {
            inf::shortcut::visualize(
                "taubin." + std::to_string(i), mp, std::make_shared<TinfMesh>(mesh));
        }
    }
}
std::shared_ptr<inf::TinfMesh> inf::SurfaceMesh::triangulate(const inf::MeshInterface& input_mesh) {
    // This only works in serial
    PARFAIT_ASSERT(input_mesh.partitionId() == 0, "Triangulate only works on serial mesh");
    auto mesh = std::make_shared<TinfMesh>(TinfMeshData(), input_mesh.partitionId());
    for (int n = 0; n < input_mesh.nodeCount(); n++) {
        mesh->mesh.points.push_back(input_mesh.node(n));
        mesh->mesh.global_node_id.push_back(input_mesh.globalNodeId(n));
        mesh->mesh.node_owner.push_back(input_mesh.nodeOwner(n));
    }
    std::vector<int> cell_nodes;
    long next_gcid = 0;
    for (int c = 0; c < input_mesh.cellCount(); c++) {
        auto type = input_mesh.cellType(c);
        switch (type) {
            case inf::MeshInterface::CellType::TRI_3: {
                input_mesh.cell(c, cell_nodes);
                for (int i = 0; i < 3; i++) {
                    mesh->mesh.cells[inf::MeshInterface::TRI_3].push_back(cell_nodes[i]);
                }
                mesh->mesh.cell_owner[inf::MeshInterface::TRI_3].push_back(input_mesh.cellOwner(c));
                mesh->mesh.cell_tags[inf::MeshInterface::TRI_3].push_back(input_mesh.cellTag(c));
                break;
            }
            case inf::MeshInterface::CellType::QUAD_4: {
                input_mesh.cell(c, cell_nodes);
                // add first tri
                mesh->mesh.cells[inf::MeshInterface::TRI_3].push_back(cell_nodes[0]);
                mesh->mesh.cells[inf::MeshInterface::TRI_3].push_back(cell_nodes[1]);
                mesh->mesh.cells[inf::MeshInterface::TRI_3].push_back(cell_nodes[2]);
                // add second tri
                mesh->mesh.cells[inf::MeshInterface::TRI_3].push_back(cell_nodes[0]);
                mesh->mesh.cells[inf::MeshInterface::TRI_3].push_back(cell_nodes[2]);
                mesh->mesh.cells[inf::MeshInterface::TRI_3].push_back(cell_nodes[3]);
                // add both sides
                mesh->mesh.global_cell_id[inf::MeshInterface::TRI_3].push_back(next_gcid++);
                mesh->mesh.global_cell_id[inf::MeshInterface::TRI_3].push_back(next_gcid++);
                mesh->mesh.cell_owner[inf::MeshInterface::TRI_3].push_back(input_mesh.cellOwner(c));
                mesh->mesh.cell_tags[inf::MeshInterface::TRI_3].push_back(input_mesh.cellTag(c));
                mesh->mesh.cell_owner[inf::MeshInterface::TRI_3].push_back(input_mesh.cellOwner(c));
                mesh->mesh.cell_tags[inf::MeshInterface::TRI_3].push_back(input_mesh.cellTag(c));
                break;
            }
            default:
                PARFAIT_THROW("Cannot triangulate with element types more than TRI or QUAD");
        }
    }
    mesh->rebuild();
    return mesh;
}

std::vector<double> inf::SurfaceMesh::aspectRatio(const inf::MeshInterface& mesh) {
    std::vector<double> aspect_ratio(mesh.cellCount());
    std::vector<int> cell_nodes;
    for (int cellid = 0; cellid < mesh.cellCount(); cellid++) {
        auto type = mesh.cellType(cellid);
        mesh.cell(cellid, cell_nodes);
        if (type == inf::MeshInterface::TRI_3) {
            Parfait::Point<double> a = mesh.node(cell_nodes[0]);
            Parfait::Point<double> b = mesh.node(cell_nodes[1]);
            Parfait::Point<double> c = mesh.node(cell_nodes[2]);
            aspect_ratio[cellid] = Parfait::Facet(a, b, c).aspectRatio();
        } else if (type == inf::MeshInterface::QUAD_4) {
            Parfait::Point<double> a = mesh.node(cell_nodes[0]);
            Parfait::Point<double> b = mesh.node(cell_nodes[1]);
            Parfait::Point<double> c = mesh.node(cell_nodes[2]);
            Parfait::Point<double> d = mesh.node(cell_nodes[3]);
            double la = (b - a).magnitude();
            double lb = (c - b).magnitude();
            double lc = (d - c).magnitude();
            double ld = (a - c).magnitude();
            double min = Parfait::VectorTools::min(std::vector<double>{la, lb, lc, ld});
            double max = Parfait::VectorTools::max(std::vector<double>{la, lb, lc, ld});
            aspect_ratio[cellid] = max / min;
        }
    }
    return aspect_ratio;
}
