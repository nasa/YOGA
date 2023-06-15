#include "MeshConnectivity.h"
#include <parfait/CGNSElements.h>
#include <parfait/CellWindingConverters.h>
#include <parfait/EdgeBuilder.h>
#include <parfait/VectorTools.h>
#include <parfait/Throw.h>
#include "BoundaryNodes.h"

using namespace inf;

std::vector<std::vector<int>> NodeToNode::buildForAnyNodeInSharedCell(
    const inf::MeshInterface& mesh) {
    std::vector<std::set<int>> n2n_set(mesh.nodeCount());
    std::vector<int> cell_nodes;
    for (int c = 0; c < mesh.cellCount(); c++) {
        mesh.cell(c, cell_nodes);
        for (auto n1 : cell_nodes) {
            for (int n2 : cell_nodes) {
                if (n1 != n2) {
                    n2n_set[n1].insert(n2);
                }
            }
        }
    }

    std::vector<std::vector<int>> n2n;
    for (auto& s : n2n_set) n2n.emplace_back(std::vector<int>(s.begin(), s.end()));
    return n2n;
}

std::vector<std::vector<int>> NodeToNode::build(const inf::MeshInterface& mesh) {
    auto edges = EdgeToNode::build(mesh);
    std::vector<std::set<int>> n2n_set(mesh.nodeCount());
    for (auto& e : edges) {
        n2n_set[e[0]].insert(e[1]);
        n2n_set[e[1]].insert(e[0]);
    }
    std::vector<std::vector<int>> n2n;
    for (auto& s : n2n_set) n2n.emplace_back(std::vector<int>(s.begin(), s.end()));
    return n2n;
}

std::vector<std::vector<int>> NodeToNode::buildSurfaceOnly(const inf::MeshInterface& mesh) {
    std::set<int> surface_nodes = BoundaryNodes::buildSet(mesh);
    auto edges = buildUniqueSurfaceEdgesForCellsTouchingNodes(mesh, surface_nodes);
    std::vector<std::set<int>> n2n_set(mesh.nodeCount());
    for (auto& e : edges) {
        n2n_set[e[0]].insert(e[1]);
        n2n_set[e[1]].insert(e[0]);
    }
    std::vector<std::vector<int>> n2n;
    for (auto& s : n2n_set) n2n.emplace_back(std::vector<int>(s.begin(), s.end()));
    return n2n;
}

std::vector<std::array<int, 2>> NodeToNode::buildUniqueSurfaceEdgesForCellsTouchingNodes(
    const inf::MeshInterface& mesh, const std::set<int>& requested_nodes) {
    auto should_build_cell = [&](const std::array<int, 8> cell) {
        for (auto n : cell) {
            if (requested_nodes.count(n) != 0) return true;
        }
        return false;
    };
    Parfait::EdgeBuilder builder;
    std::array<int, 8> cell;
    for (int c = 0; c < mesh.cellCount(); c++) {
        mesh.cell(c, cell.data());
        if (not should_build_cell(cell)) continue;
        auto type = mesh.cellType(c);
        switch (type) {
            case inf::MeshInterface::TETRA_4:
            case inf::MeshInterface::PENTA_6:
            case inf::MeshInterface::PYRA_5:
            case inf::MeshInterface::HEXA_8:
                break;
            case inf::MeshInterface::BAR_2: {
                std::array<std::array<int, 2>, 1> edge_to_node = {{{0, 1}}};
                builder.addCell(cell.data(), edge_to_node);
                break;
            }
            case inf::MeshInterface::TRI_3:
                builder.addCell(cell.data(), Parfait::CGNS::Triangle::edge_to_node);
                break;
            case inf::MeshInterface::QUAD_4:
                builder.addCell(cell.data(), Parfait::CGNS::Quad::edge_to_node);
                break;
            default:
                throw std::logic_error("Only linear types for edge builder");
        }
    }
    return builder.edges();
}

std::vector<std::vector<int>> NodeToCell::build(const MeshInterface& mesh) {
    std::vector<std::vector<int>> node_to_cell(mesh.nodeCount());
    std::vector<int> cell;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        auto type = mesh.cellType(cell_id);
        cell.resize(MeshInterface::cellTypeLength(type));
        mesh.cell(cell_id, cell.data());
        for (int node : cell) {
            node_to_cell[node].push_back(cell_id);
        }
    }
    return node_to_cell;
}
std::vector<std::vector<int>> NodeToCell::buildVolumeOnly(const inf::MeshInterface& mesh) {
    return buildDimensionOnly(mesh, 3);
}
std::vector<std::vector<int>> NodeToCell::buildSurfaceOnly(const inf::MeshInterface& mesh) {
    return buildDimensionOnly(mesh, 2);
}
std::vector<std::vector<int>> NodeToCell::buildDimensionOnly(const MeshInterface& mesh,
                                                             int dimension) {
    std::vector<std::vector<int>> node_to_cell(mesh.nodeCount());
    std::vector<int> cell;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        auto type = mesh.cellType(cell_id);
        if (MeshInterface::cellTypeDimensionality(type) == dimension) {
            cell.resize(MeshInterface::cellTypeLength(type));
            mesh.cell(cell_id, cell.data());
            for (int node : cell) {
                node_to_cell[node].push_back(cell_id);
            }
        }
    }
    return node_to_cell;
}
std::vector<std::vector<int>> CellToCell::build(const MeshInterface& mesh,
                                                const std::vector<std::vector<int>>& n2c) {
    std::vector<std::vector<int>> c2c(mesh.cellCount());
    std::vector<int> nodes;
    nodes.reserve(8);

    for (int c = 0; c < mesh.cellCount(); c++) {
        auto type = mesh.cellType(c);
        int length = mesh.cellTypeLength(type);
        nodes.resize(length);
        mesh.cell(c, nodes.data());
        for (auto node : nodes) {
            for (auto neighbor : n2c[node]) {
                if (neighbor != c) {
                    Parfait::VectorTools::insertUnique(c2c[c], neighbor);
                }
            }
        }
    }
    return c2c;
}

std::vector<std::vector<int>> CellToCell::build(const inf::MeshInterface& mesh) {
    return build(mesh, NodeToCell::build(mesh));
}

std::vector<std::vector<int>> CellToCell::buildDimensionOnly(
    const MeshInterface& mesh, const std::vector<std::vector<int>>& n2c, int dimension) {
    std::vector<std::vector<int>> c2c(mesh.cellCount());
    std::vector<int> nodes;
    nodes.reserve(8);

    for (int c = 0; c < mesh.cellCount(); c++) {
        auto type = mesh.cellType(c);
        if (MeshInterface::cellTypeDimensionality(type) == dimension) {
            int length = MeshInterface::cellTypeLength(type);
            nodes.resize(length);
            mesh.cell(c, nodes.data());
            for (int node : nodes) {
                for (const auto& neighbor : n2c[node]) {
                    if (neighbor != c) {
                        if (mesh.cellDimensionality(neighbor) == dimension) {
                            Parfait::VectorTools::insertUnique(c2c[c], neighbor);
                        }
                    }
                }
            }
        }
    }
    return c2c;
}

std::vector<std::vector<int>> CellToCell::buildDimensionOnly(const MeshInterface& mesh,
                                                             int dimension) {
    return buildDimensionOnly(mesh, NodeToCell::buildDimensionOnly(mesh, dimension), dimension);
}
std::vector<std::vector<int>> EdgeToCell::build(const MeshInterface& mesh,
                                                const std::vector<std::array<int, 2>>& edges) {
    auto node_to_cell = NodeToCell::buildVolumeOnly(mesh);
    std::vector<std::set<int>> edge_to_cell(edges.size());
    for (size_t edge_id = 0; edge_id < edges.size(); ++edge_id) {
        int left_node = edges[edge_id][0];
        int right_node = edges[edge_id][1];
        for (int cell_id : node_to_cell[left_node]) {
            if (Parfait::VectorTools::isIn(node_to_cell[right_node], cell_id)) {
                edge_to_cell[edge_id].insert(cell_id);
            }
        }
    }
    std::vector<std::vector<int>> e2c(edges.size());
    for (size_t edge_id = 0; edge_id < e2c.size(); ++edge_id)
        e2c[edge_id] = std::vector<int>(edge_to_cell[edge_id].begin(), edge_to_cell[edge_id].end());

    return e2c;
}
std::vector<std::vector<int>> EdgeToCellNodes::build(const MeshInterface& mesh,
                                                     const std::vector<std::array<int, 2>>& edges) {
    auto edge_to_cell = EdgeToCell::build(mesh, edges);
    std::vector<std::set<int>> edge_to_cell_nodes(edges.size());
    for (size_t edge_id = 0; edge_id < edges.size(); ++edge_id) {
        for (int cell_id : edge_to_cell[edge_id]) {
            std::vector<int> cell_nodes(mesh.cellLength(cell_id));
            mesh.cell(cell_id, cell_nodes.data());
            edge_to_cell_nodes[edge_id].insert(cell_nodes.begin(), cell_nodes.end());
        }
    }
    std::vector<std::vector<int>> e2c(edges.size());
    for (size_t edge_id = 0; edge_id < e2c.size(); ++edge_id)
        e2c[edge_id] = std::vector<int>(edge_to_cell_nodes[edge_id].begin(),
                                        edge_to_cell_nodes[edge_id].end());

    return e2c;
}
std::vector<std::array<int, 2>> EdgeToNode::build(const MeshInterface& mesh) {
    Parfait::EdgeBuilder builder;
    std::array<int, 8> cell;
    for (int c = 0; c < mesh.cellCount(); c++) {
        mesh.cell(c, cell.data());
        auto type = mesh.cellType(c);
        switch (type) {
            case MeshInterface::TETRA_4:
                builder.addCell(cell.data(), Parfait::CGNS::Tet::edge_to_node);
                break;
            case MeshInterface::PENTA_6:
                builder.addCell(cell.data(), Parfait::CGNS::Prism::edge_to_node);
                break;
            case MeshInterface::PYRA_5:
                builder.addCell(cell.data(), Parfait::CGNS::Pyramid::edge_to_node);
                break;
            case MeshInterface::HEXA_8:
                builder.addCell(cell.data(), Parfait::CGNS::Hex::edge_to_node);
                break;
            case MeshInterface::BAR_2:
                builder.addEdge(cell[0], cell[1]);
                break;
            case MeshInterface::TRI_3:
                builder.addCell(cell.data(), Parfait::CGNS::Triangle::edge_to_node);
                break;
            case MeshInterface::QUAD_4:
                builder.addCell(cell.data(), Parfait::CGNS::Quad::edge_to_node);
                break;
            default:
                PARFAIT_THROW("buildUniqueEdges only supports linear element types");
        }
    }
    return builder.edges();
}
std::vector<std::array<int, 2>> EdgeToNode::buildOnSurface(const MeshInterface& mesh,
                                                           std::set<int> surface_tags) {
    Parfait::EdgeBuilder builder;
    std::array<int, 8> cell;
    for (int c = 0; c < mesh.cellCount(); c++) {
        mesh.cell(c, cell.data());
        auto type = mesh.cellType(c);
        auto tag = mesh.cellTag(c);
        if (inf::MeshInterface::is2DCellType(type) and surface_tags.count(tag) == 1) {
            switch (type) {
                case MeshInterface::BAR_2:
                    builder.addEdge(cell.data()[0], cell.data()[1]);
                    break;
                case MeshInterface::TRI_3:
                    builder.addCell(cell.data(), Parfait::CGNS::Triangle::edge_to_node);
                    break;
                case MeshInterface::QUAD_4:
                    builder.addCell(cell.data(), Parfait::CGNS::Quad::edge_to_node);
                    break;
                default:
                    PARFAIT_THROW("buildUniqueEdges only supports linear element types");
            }
        }
    }
    return builder.edges();
}
