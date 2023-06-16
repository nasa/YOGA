#include <parfait/Normals.h>
#include <parfait/VectorTools.h>
#include "MeshExtruder.h"
#include "QuiltTags.h"
#include "MeshBuilder.h"
#include "AddMissingFaces.h"
#include "GlobalToLocal.h"
#include "SurfaceElementWinding.h"
#include "SurfaceNeighbors.h"
#include "Shortcuts.h"

using namespace inf;

class Extruder {
  public:
    Extruder(MessagePasser mp, double tolerance = 1.0e-8)
        : mp(mp),
          tolerance(tolerance),
          builder(std::make_unique<inf::experimental::MeshBuilder>(mp)) {}

    std::map<int, std::string> buildOldTagsToNonDefaultNames(const inf::MeshInterface& mesh) {
        auto otn = inf::extract1DTagsToNames(mp, mesh);
        std::map<int, std::string> old_tags_to_names;
        old_tags_to_names[symmetry_plane_tag_1] = "symm-1";
        old_tags_to_names[symmetry_plane_tag_2] = "symm-2";

        for (auto& pair : otn) {
            auto old = pair.first;
            auto name = pair.second;
            if (name != "Tag_" + std::to_string(old)) old_tags_to_names[old] = name;
        }
        return old_tags_to_names;
    }

    void setSafeSymmetryTags(const MeshInterface& mesh) {
        auto tags = inf::extractAllTagsWithDimensionality(mp, mesh, 1);
        while (tags.count(symmetry_plane_tag_1)) symmetry_plane_tag_1++;
        symmetry_plane_tag_2 = symmetry_plane_tag_1 + 1;
        while (tags.count(symmetry_plane_tag_2)) symmetry_plane_tag_2++;
    }
    std::shared_ptr<inf::TinfMesh> extrudeAxisymmetric(const inf::MeshInterface& mesh,
                                                       inf::extrude::Type type,
                                                       double angle_in_degrees,
                                                       bool symmetric) {
        setSafeSymmetryTags(mesh);
        extrudeNodesAxisymmetric(mesh, type, angle_in_degrees, symmetric);
        extrudeCells(mesh);

        auto old_tags_to_names = buildOldTagsToNonDefaultNames(mesh);
        builder->sync();
        output_mesh = builder->mesh;
        builder.reset(nullptr);

        addMissingFacesAndGiveThemCartesianNormalBasedTags();

        auto old_to_new_tags = forceTagsToBeOrdinalSet();
        for (auto pair : old_to_new_tags) {
            auto old_tag = pair.first;
            auto new_tag = pair.second;
            if (old_tags_to_names.count(old_tag)) {
                auto tag_name = old_tags_to_names.at(old_tag);
                output_mesh->setTagName(new_tag, tag_name);
            }
        }
        return output();
    }

    void extrudeCells(const inf::MeshInterface& orig_mesh) {
        std::vector<int> cells_to_extrude;
        for (int c = 0; c < orig_mesh.cellCount(); c++) {
            if (orig_mesh.cellDimensionality(c) > 0) cells_to_extrude.push_back(c);
        }
        std::vector<int> cell_nodes;
        for (int c : cells_to_extrude) {
            auto type = orig_mesh.cellType(c);
            cell_nodes.resize(inf::MeshInterface::cellTypeLength(type));
            orig_mesh.cell(c, cell_nodes.data());
            switch (type) {
                case inf::MeshInterface::BAR_2: {
                    extrude::Cell cell = extrude::extrudeBar(cell_nodes, node_old_to_new);
                    if (cell.is_null) break;
                    builder->addCell(cell.type, cell.cell_nodes, orig_mesh.cellTag(c));
                    break;
                }
                case inf::MeshInterface::TRI_3: {
                    extrude::Cell volume_cell =
                        extrude::extrudeTriangle(cell_nodes, node_old_to_new);
                    int volume_cell_tag = 0;
                    builder->addCell(volume_cell.type, volume_cell.cell_nodes, volume_cell_tag);
                    builder->addCell(inf::MeshInterface::TRI_3, cell_nodes, symmetry_plane_tag_1);
                    for (auto& n : cell_nodes) {
                        n = node_old_to_new[n];
                    }
                    builder->addCell(inf::MeshInterface::TRI_3, cell_nodes, symmetry_plane_tag_2);
                    break;
                }
                default:
                    PARFAIT_THROW("Cannot extrude cell type " +
                                  inf::MeshInterface::cellTypeString(type) +
                                  "Contact Matthew.David.OConnell@nasa.gov");
            }
        }
        builder->sync();
    }

    void extrudeNodesAxisymmetric(const inf::MeshInterface& orig_mesh,
                                  extrude::Type type,
                                  double angle_degrees,
                                  bool symmetric) {
        PARFAIT_ASSERT(type == extrude::Type::AXISYMMETRIC_X, "Can only extrude about x axis");
        double angle = angle_degrees * M_PI / 180.0;
        if (symmetric) angle *= 0.5;
        auto forward_mover = [=](const Parfait::Point<double>& p) {
            auto o = p;
            o[1] = cos(angle) * p[1] - sin(angle) * p[2];
            o[2] = sin(angle) * p[1] + cos(angle) * p[2];
            return o;
        };

        auto backward_mover = [=](const Parfait::Point<double>& p) {
            auto o = p;
            double a = -1.0 * angle;
            o[1] = cos(a) * p[1] - sin(a) * p[2];
            o[2] = sin(a) * p[1] + cos(a) * p[2];
            return o;
        };

        node_old_to_new.resize(orig_mesh.nodeCount());

        for (int n = 0; n < orig_mesh.nodeCount(); n++) {
            PARFAIT_ASSERT(orig_mesh.globalNodeId(n) == n, "must be in GID order");
            Parfait::Point<double> p = orig_mesh.node(n);
            builder->addNode(p);
        }

        for (int n = 0; n < orig_mesh.nodeCount(); n++) {
            Parfait::Point<double> orig_p = orig_mesh.node(n);
            auto p = forward_mover(orig_p);
            double d = Parfait::Point<double>::distance(p, orig_p);
            if (d < tolerance) {
                node_old_to_new[n] = n;
            } else {
                int new_node_id = builder->addNode(p);
                node_old_to_new[n] = new_node_id;
                if (symmetric) {
                    builder->mesh->mesh.points[n] = backward_mover(orig_p);
                }
            }
        }
    }

    std::shared_ptr<TinfMesh> output() { return std::move(output_mesh); }

    void addMissingFacesAndGiveThemCartesianNormalBasedTags() {
        output_mesh = inf::addMissingFaces(mp, *output_mesh, missing_face_tag);
        auto surface_neighbors = SurfaceNeighbors::buildSurfaceNeighbors(output_mesh);
        SurfaceElementWinding::windAllSurfaceElementsOut(mp, *output_mesh, surface_neighbors);

        auto all_tags = inf::extractAll2DTags(mp, *output_mesh);
        int max_current_tag = *std::max_element(all_tags.begin(), all_tags.end());
        auto tags_to_normals = inf::getCartesianNormalTags(max_current_tag);
        inf::relabelSurfaceTagsByNearestNormal(*output_mesh, tags_to_normals, {missing_face_tag});
    }

    std::map<int, int> forceTagsToBeOrdinalSet() {
        return inf::quiltTagsAndCompact(mp, *output_mesh, {});
    }

  private:
    int missing_face_tag = -123456;
    int symmetry_plane_tag_1 = 10174927;
    int symmetry_plane_tag_2 = 10174928;
    MessagePasser mp;
    double tolerance;
    std::unique_ptr<inf::experimental::MeshBuilder> builder;
    std::shared_ptr<TinfMesh> output_mesh = nullptr;
    std::vector<int> node_old_to_new;
};

std::shared_ptr<inf::TinfMesh> reorderMeshNodesSoLIDEqualsGID(const inf::MeshInterface& m) {
    auto tinf_mesh = std::make_shared<inf::TinfMesh>(m);
    auto g2l = inf::GlobalToLocal::buildNode(m);
    for (int n = 0; n < m.nodeCount(); n++) {
        tinf_mesh->mesh.global_node_id[n] = n;
        tinf_mesh->mesh.points[n] = m.node(g2l.at(n));
    }
    for (auto& pair : tinf_mesh->mesh.cells) {
        auto& cell_nodes = pair.second;
        for (auto& n : cell_nodes) {
            n = m.globalNodeId(n);
        }
    }
    return tinf_mesh;
}

std::shared_ptr<inf::TinfMesh> inf::extrude::extrudeAxisymmetric(MessagePasser mp,
                                                                 const inf::MeshInterface& m,
                                                                 inf::extrude::Type type,
                                                                 double angle_in_degrees,
                                                                 bool symmetric) {
    PARFAIT_ASSERT(mp.NumberOfProcesses() == 1, "Cannot extrude meshes in MPI parallel");

    auto tinf_mesh = reorderMeshNodesSoLIDEqualsGID(m);
    auto extruder = Extruder(mp);
    return extruder.extrudeAxisymmetric(*tinf_mesh, type, angle_in_degrees, symmetric);
}

extrude::Cell extrude::extrudeBar(std::vector<int> cell_nodes,
                                  const std::vector<int>& old_to_new_nodes) {
    for (int i = 0; i < 2; i++) {
        cell_nodes.push_back(old_to_new_nodes[cell_nodes[i]]);
    }

    auto cell_nodes_unique =
        Parfait::VectorTools::toVector(std::set<int>(cell_nodes.begin(), cell_nodes.end()));
    int num_unique = cell_nodes_unique.size();
    PARFAIT_ASSERT(num_unique > 1, "Found degenerate cell while extruding BAR");

    if (num_unique == 2) {
        Cell cell;
        cell.is_null = true;
        cell.type = inf::MeshInterface::NULL_CELL;
        return cell;
    }

    if (num_unique == 3) {
        Cell cell;
        cell.type = inf::MeshInterface::TRI_3;
        cell.cell_nodes = cell_nodes_unique;
        return cell;
    }

    if (num_unique == 4) {
        Cell cell;
        cell.type = inf::MeshInterface::QUAD_4;
        std::swap(cell_nodes[2], cell_nodes[3]);
        cell.cell_nodes = cell_nodes;
        return cell;
    }

    PARFAIT_THROW("Uncovered bar extrusion path found");
    return Cell();
}

extrude::Cell extrude::extrudeTriangle(std::vector<int> cell_nodes,
                                       const std::vector<int>& old_to_new_nodes) {
    for (int i = 0; i < 3; i++) {
        cell_nodes.push_back(old_to_new_nodes[cell_nodes[i]]);
    }

    int num_unique = int(std::set<int>(cell_nodes.begin(), cell_nodes.end()).size());

    if (num_unique == 6) {
        Cell cell;
        cell.type = inf::MeshInterface::PENTA_6;
        cell.cell_nodes = cell_nodes;
        return cell;
    }

    if (num_unique == 5) {
        Cell cell;
        cell.type = inf::MeshInterface::PYRA_5;
        cell.cell_nodes.resize(5);
        if (cell_nodes[0] == cell_nodes[3]) {
            cell.cell_nodes[0] = cell_nodes[1];
            cell.cell_nodes[1] = cell_nodes[4];
            cell.cell_nodes[2] = cell_nodes[5];
            cell.cell_nodes[3] = cell_nodes[2];
            cell.cell_nodes[4] = cell_nodes[0];
        } else if (cell_nodes[1] == cell_nodes[4]) {
            cell.cell_nodes[0] = cell_nodes[2];
            cell.cell_nodes[1] = cell_nodes[5];
            cell.cell_nodes[2] = cell_nodes[3];
            cell.cell_nodes[3] = cell_nodes[0];
            cell.cell_nodes[4] = cell_nodes[1];
        } else if (cell_nodes[2] == cell_nodes[5]) {
            cell.cell_nodes[0] = cell_nodes[0];
            cell.cell_nodes[1] = cell_nodes[3];
            cell.cell_nodes[2] = cell_nodes[4];
            cell.cell_nodes[3] = cell_nodes[1];
            cell.cell_nodes[4] = cell_nodes[2];
        }
        return cell;
    }
    if (num_unique == 4) {
        Cell cell;
        cell.type = inf::MeshInterface::TETRA_4;
        cell.cell_nodes.resize(4);
        if (cell_nodes[2] != cell_nodes[5]) {
            cell.cell_nodes[0] = cell_nodes[0];
            cell.cell_nodes[1] = cell_nodes[1];
            cell.cell_nodes[2] = cell_nodes[2];
            cell.cell_nodes[3] = cell_nodes[5];
        } else if (cell_nodes[0] != cell_nodes[3]) {
            cell.cell_nodes[0] = cell_nodes[0];
            cell.cell_nodes[1] = cell_nodes[1];
            cell.cell_nodes[2] = cell_nodes[2];
            cell.cell_nodes[3] = cell_nodes[3];
        } else if (cell_nodes[2] == cell_nodes[5]) {
            cell.cell_nodes[0] = cell_nodes[1];
            cell.cell_nodes[1] = cell_nodes[2];
            cell.cell_nodes[2] = cell_nodes[0];
            cell.cell_nodes[3] = cell_nodes[4];
        }
        return cell;
    }

    PARFAIT_THROW("Uncovered triangle extrusion path found.  Nodes: " +
                  Parfait::to_string(cell_nodes));
    return Cell();
}
Parfait::Point<double> computeCellNormal(const inf::experimental::MeshBuilder& builder,
                                         inf::MeshInterface::CellType type,
                                         int index) {
    auto cell = builder.cell(type, index);

    Parfait::Point<double> a = builder.mesh->node(cell[0]);
    Parfait::Point<double> b = builder.mesh->node(cell[1]);
    Parfait::Point<double> c = builder.mesh->node(cell[2]);
    Parfait::Point<double> normal = Parfait::Facet(a, b, c).computeNormal();
    if (type == inf::MeshInterface::QUAD_4) {
        a = builder.mesh->node(cell[2]);
        b = builder.mesh->node(cell[3]);
        c = builder.mesh->node(cell[0]);
        normal = 0.5 * (normal + Parfait::Facet(a, b, c).computeNormal());
    }
    return normal;
}

void extrude::extrudeTags(inf::experimental::MeshBuilder& builder,
                          std::set<int> tags_to_extrude,
                          bool erase_extruded_surfaces,
                          bool extrude_reverse,
                          double spacing,
                          int new_tag) {
    int num_nodes = builder.mesh->nodeCount();
    std::vector<Parfait::Point<double>> normals(num_nodes);
    std::vector<Parfait::Point<double>> temp_normals;
    std::set<inf::experimental::CellEntry> cells_to_extrude;
    std::set<int> nodes_to_extrude;
    for (int n = 0; n < num_nodes; n++) {
        temp_normals.clear();
        for (auto neighbor : builder.node_to_cells[n]) {
            int tag = builder.mesh->mesh.cell_tags[neighbor.type][neighbor.index];
            if (builder.mesh->is2DCellType(neighbor.type) and tags_to_extrude.count(tag) == 1) {
                cells_to_extrude.insert(neighbor);
                temp_normals.push_back(computeCellNormal(builder, neighbor.type, neighbor.index));
                nodes_to_extrude.insert(n);
            }
        }
        normals[n] = Parfait::Normals::average(temp_normals);
        if (extrude_reverse) {
            normals[n] *= -1.0;
        }
    }

    double dx = spacing;
    auto& points = builder.mesh->mesh.points;
    points.reserve(num_nodes * 2);
    std::vector<int> bottom_to_top_nodes;
    bottom_to_top_nodes.resize(num_nodes);
    for (auto n : nodes_to_extrude) {
        auto p = points[n] + dx * normals[n];
        bottom_to_top_nodes[n] = builder.addNode(p);
    }

    std::vector<int> prism(6);
    std::vector<int> tri(3);
    std::vector<int> hex(8);
    std::vector<int> quad(4);
    for (auto& c : cells_to_extrude) {
        if (c.type == inf::MeshInterface::TRI_3) {
            auto& triangles = builder.mesh->mesh.cells[inf::MeshInterface::TRI_3];
            int t = c.index;
            for (int i = 0; i < 3; i++) {
                prism[i] = triangles[3 * t + i];
                prism[i + 3] = bottom_to_top_nodes[prism[i]];
                tri[i] = prism[i + 3];
            }
            if (erase_extruded_surfaces) {
                builder.deleteCell(c.type, c.index);
            }
            if (extrude_reverse) {
                std::reverse(tri.begin(), tri.end());
                std::reverse(prism.begin(), prism.begin() + 3);
                std::reverse(prism.begin() + 3, prism.end());
            }
            builder.addCell(inf::MeshInterface::PENTA_6, prism, 0);
            builder.addCell(inf::MeshInterface::TRI_3, tri, new_tag);
        } else if (c.type == inf::MeshInterface::QUAD_4) {
            auto& quads = builder.mesh->mesh.cells[inf::MeshInterface::QUAD_4];
            int q = c.index;
            for (int i = 0; i < 4; i++) {
                hex[i] = quads.at(4 * q + i);
                hex[i + 4] = bottom_to_top_nodes[hex[i]];
                quad[i] = hex[i + 4];
            }
            if (erase_extruded_surfaces) {
                builder.deleteCell(c.type, c.index);
            }
            if (extrude_reverse) {
                std::reverse(quad.begin(), quad.end());
                std::reverse(hex.begin(), hex.begin() + 4);
                std::reverse(hex.begin() + 4, hex.end());
            }
            builder.addCell(inf::MeshInterface::HEXA_8, hex, 0);
            builder.addCell(inf::MeshInterface::QUAD_4, quad, new_tag);
        }
    }
    builder.sync();
}
std::shared_ptr<TinfMesh> extrude::extrudeTags(const MeshInterface& m,
                                               std::set<int> tags,
                                               double spacing,
                                               int nlayers,
                                               bool both_sides,
                                               int new_tag) {
    auto mp = MessagePasser(MPI_COMM_SELF);
    inf::experimental::MeshBuilder builder(mp, m);

    bool erase = false;
    bool reverse = false;
    inf::extrude::extrudeTags(builder, tags, erase, reverse, spacing, new_tag);
    if (both_sides) {
        erase = true;
        reverse = true;
        inf::extrude::extrudeTags(builder, tags, erase, reverse, spacing, new_tag);
    }

    tags = std::set<int>{new_tag};
    erase = true;
    reverse = false;
    for (int n = 1; n < nlayers; n++) {
        inf::extrude::extrudeTags(builder, tags, erase, reverse, spacing, new_tag);
    }

    return builder.mesh;
}
