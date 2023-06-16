#include "Cell.h"
#include <t-infinity/MeshInterface.h>
#include "InfinityToVtk.h"
#include <parfait/Throw.h>
#include <t-infinity/FaceNeighbors.h>
#include <parfait/CGNSFaceExtraction.h>
#include <parfait/Metrics.h>
#include <parfait/TecplotWriter.h>
#include <parfait/VTKWriter.h>
#include <parfait/ExtentBuilder.h>
#include "Hiro.h"
#include <parfait/CGNSElements.h>

inf::Cell::Cell(const inf::MeshInterface& mesh, int cell_id, int d)
    : dimension_mode(d), cell_type(mesh.cellType(cell_id)) {
    int length = inf::MeshInterface::cellTypeLength(mesh.cellType(cell_id));
    cell_nodes.resize(length);
    mesh.cell(cell_id, cell_nodes.data());
    for (int n : cell_nodes) {
        PARFAIT_ASSERT(n >= 0, "Cell has negative node.  Was this cell lazy deleted?");
        auto p = mesh.node(n);
        cell_points.push_back(p);
    }
}
inf::Cell::Cell(const inf::TinfMesh& mesh,
                inf::MeshInterface::CellType type,
                int cell_index_in_type,
                int mesh_dimension)
    : dimension_mode(mesh_dimension), cell_type(type) {
    int cell_length = inf::MeshInterface::cellTypeLength(type);
    cell_nodes.resize(cell_length);
    cell_points.resize(cell_length);
    for (int i = 0; i < cell_length; i++) {
        PARFAIT_ASSERT(mesh.mesh.cells.count(type) == 1,
                       "no cells of type " + inf::MeshInterface::cellTypeString(type) + " found");
        cell_nodes[i] = mesh.mesh.cells.at(type)[cell_length * cell_index_in_type + i];
        PARFAIT_ASSERT(cell_nodes[i] >= 0, "Cell has negative node.  Was this cell lazy deleted?");
        cell_points[i] = mesh.mesh.points[cell_nodes[i]];
    }
}
inf::Cell::Cell(std::shared_ptr<inf::MeshInterface> mesh, int cell_id, int d)
    : Cell(*mesh, cell_id, d) {}
inf::Cell::Cell(inf::MeshInterface::CellType type,
                const int* cell_nodes_in,
                const Parfait::Point<double>* cell_points_in,
                int d)
    : dimension_mode(d), cell_type(type) {
    int length = inf::MeshInterface::cellTypeLength(cell_type);
    cell_nodes.resize(length);
    cell_points.resize(length);
    for (int i = 0; i < length; i++) {
        cell_nodes[i] = cell_nodes_in[i];
        PARFAIT_ASSERT(cell_nodes[i] >= 0, "Cell has negative node.  Was this cell lazy deleted?");
        cell_points[i] = cell_points_in[i];
    }
}
inf::Cell::Cell(inf::MeshInterface::CellType type,
                const std::vector<int>& cell_nodes,
                const std::vector<Parfait::Point<double>>& cell_points,
                int d)
    : dimension_mode(d), cell_type(type), cell_nodes(cell_nodes), cell_points(cell_points) {}
inf::MeshInterface::CellType inf::Cell::type() const { return cell_type; }
std::vector<int> inf::Cell::nodes() const { return cell_nodes; }
void inf::Cell::nodes(int* cell_nodes_output) const {
    std::copy(cell_nodes.begin(), cell_nodes.end(), cell_nodes_output);
}
std::vector<std::array<int, 2>> inf::Cell::edges() const {
    if (cell_type == MeshInterface::TETRA_4) {
        auto edge_nodes = Parfait::CGNS::Tet::edge_to_node;
        std::vector<std::array<int, 2>> edges;
        for (auto& edge : edge_nodes) {
            edges.push_back({cell_nodes[edge[0]], cell_nodes[edge[1]]});
        }
        return edges;
    } else if (cell_type == MeshInterface::PYRA_5) {
        auto edge_nodes = Parfait::CGNS::Pyramid::edge_to_node;
        std::vector<std::array<int, 2>> edges;
        for (auto& edge : edge_nodes) {
            edges.push_back({cell_nodes[edge[0]], cell_nodes[edge[1]]});
        }
        return edges;
    } else if (cell_type == MeshInterface::PENTA_6) {
        auto edge_nodes = Parfait::CGNS::Prism::edge_to_node;
        std::vector<std::array<int, 2>> edges;
        for (auto& edge : edge_nodes) {
            edges.push_back({cell_nodes[edge[0]], cell_nodes[edge[1]]});
        }
        return edges;
    } else if (cell_type == MeshInterface::HEXA_8) {
        auto edge_nodes = Parfait::CGNS::Hex::edge_to_node;
        std::vector<std::array<int, 2>> edges;
        for (auto& edge : edge_nodes) {
            edges.push_back({cell_nodes[edge[0]], cell_nodes[edge[1]]});
        }
        return edges;
    } else if (cell_type == MeshInterface::TRI_3) {
        auto edge_nodes = Parfait::CGNS::Triangle::edge_to_node;
        std::vector<std::array<int, 2>> edges;
        for (auto& edge : edge_nodes) {
            edges.push_back({cell_nodes[edge[0]], cell_nodes[edge[1]]});
        }
        return edges;
    } else if (cell_type == MeshInterface::QUAD_4) {
        auto edge_nodes = Parfait::CGNS::Quad::edge_to_node;
        std::vector<std::array<int, 2>> edges;
        for (auto& edge : edge_nodes) {
            edges.push_back({cell_nodes[edge[0]], cell_nodes[edge[1]]});
        }
        return edges;
    } else if (cell_type == MeshInterface::BAR_2) {
        return {{cell_nodes[0], cell_nodes[1]}};
    } else if (cell_type == MeshInterface::NODE) {
        return {};
    } else {
        PARFAIT_THROW("edgeCount not implemented for type: " +
                      MeshInterface::cellTypeString(cell_type));
    }
}
int inf::Cell::edgeCount() const {
    if (cell_type == MeshInterface::TETRA_4)
        return 6;
    else if (cell_type == MeshInterface::PYRA_5)
        return 8;
    else if (cell_type == MeshInterface::PENTA_6)
        return 9;
    else if (cell_type == MeshInterface::HEXA_8)
        return 12;
    else if (cell_type == MeshInterface::TRI_3)
        return 3;
    else if (cell_type == MeshInterface::QUAD_4)
        return 4;
    else
        PARFAIT_THROW("edgeCount not implemented for type: " +
                      MeshInterface::cellTypeString(cell_type));
}
int inf::Cell::faceCount2D() const {
    switch (cell_type) {
        case MeshInterface::TRI_3:
            return 3;
        case MeshInterface::QUAD_4:
            return 4;
        case MeshInterface::BAR_2:
            return 1;
        case MeshInterface::NODE:
            return 0;
        default:
            PARFAIT_THROW("faceCount not implemented for type: " +
                          MeshInterface::cellTypeString(cell_type));
    }
}
int inf::Cell::faceCount() const {
    if (dimension_mode == 2) {
        return faceCount2D();
    }
    using namespace inf;
    switch (cell_type) {
        case MeshInterface::TETRA_4:
            return 4;
        case MeshInterface::PYRA_5:
            return 5;
        case MeshInterface::PENTA_6:
            return 5;
        case MeshInterface::HEXA_8:
            return 6;
        case MeshInterface::TRI_3:
            return 1;
        case MeshInterface::QUAD_4:
            return 1;
        case MeshInterface::BAR_2:
            return 0;
        case MeshInterface::NODE:
            return 0;
        default:
            PARFAIT_THROW("faceCount not implemented for type: " +
                          MeshInterface::cellTypeString(cell_type));
    }
}
std::vector<int> inf::Cell::faceNodes(int face_number,
                                      MeshInterface::CellType cell_type,
                                      const std::vector<int>& cell_nodes,
                                      int dimension_mode) {
    if (dimension_mode == 2) {
        std::vector<int> face_nodes;
        FaceNeighbors::getFace2D(cell_type, cell_nodes, face_number, face_nodes);
        return face_nodes;
    }
    std::vector<int> face_nodes;
    switch (cell_type) {
        case inf::MeshInterface::TRI_3:
        case inf::MeshInterface::QUAD_4:
            return cell_nodes;
        case inf::MeshInterface::TETRA_4: {
            Parfait::CGNS::getTetFace(cell_nodes, face_number, face_nodes);
            return face_nodes;
        }
        case inf::MeshInterface::PYRA_5: {
            Parfait::CGNS::getPyramidFace(cell_nodes, face_number, face_nodes);
            return face_nodes;
        }
        case inf::MeshInterface::PENTA_6: {
            Parfait::CGNS::getPrismFace(cell_nodes, face_number, face_nodes);
            return face_nodes;
        }
        case inf::MeshInterface::HEXA_8: {
            Parfait::CGNS::getHexFace(cell_nodes, face_number, face_nodes);
            return face_nodes;
        }
        default:
            PARFAIT_THROW("Cell::face not implemented for type: " +
                          MeshInterface::cellTypeString(cell_type));
    }
}
std::vector<int> inf::Cell::faceNodes(int face_number) const {
    return faceNodes(face_number, cell_type, nodes(), dimension_mode);
}
std::vector<int> inf::Cell::faceNodes2D(int face_number) const {
    return faceNodes(face_number, cell_type, nodes(), 2);
}
std::vector<Parfait::Point<double>> inf::Cell::points() const { return cell_points; }
Parfait::Extent<double> inf::Cell::extent() const {
    auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    for (auto p : cell_points) Parfait::ExtentBuilder::add(e, p);
    return e;
}
void inf::Cell::points(std::vector<Parfait::Point<double>>& cell_points_out) const {
    cell_points_out = cell_points;
}
Parfait::Point<double> inf::Cell::averageCenter() const {
    return Parfait::Metrics::average(points());
}
Parfait::Point<double> inf::Cell::nishikawaCentroid(double p) const {
    return Hiro::calcCentroid(*this, p);
}
Parfait::Point<double> inf::Cell::faceAverageCenter(int face_number) const {
    return Parfait::Metrics::average(facePoints(face_number));
}
std::vector<Parfait::Point<double>> inf::Cell::facePoints(int face_number) const {
    if (dimension_mode == 2) return facePoints2D(face_number);
    std::vector<Parfait::Point<double>> face_points;
    switch (cell_type) {
        case inf::MeshInterface::TRI_3:
        case inf::MeshInterface::QUAD_4:
            return points();
        case inf::MeshInterface::TETRA_4: {
            Parfait::CGNS::getTetFace(points(), face_number, face_points);
            return face_points;
        }
        case inf::MeshInterface::PYRA_5: {
            Parfait::CGNS::getPyramidFace(points(), face_number, face_points);
            return face_points;
        }
        case inf::MeshInterface::PENTA_6: {
            Parfait::CGNS::getPrismFace(points(), face_number, face_points);
            return face_points;
        }
        case inf::MeshInterface::HEXA_8: {
            Parfait::CGNS::getHexFace(points(), face_number, face_points);
            return face_points;
        }
        default:
            PARFAIT_THROW("Cell::face not implemented for type: " +
                          MeshInterface::cellTypeString(cell_type));
    }
}
Parfait::Point<double> inf::Cell::faceAreaNormal2D(int face_number) const {
    auto face_points = facePoints(face_number);
    PARFAIT_ASSERT(
        face_points.size() == 2,
        "Expected 2D face would be length 2 but is length: " + std::to_string(face_points.size()));
    auto p1 = face_points[0];
    auto p2 = face_points[1];
    auto v1 = p2 - p1;
    auto v2 = Parfait::Point<double>{0, 0, 1};
    auto v3 = v1.cross(v2);
    return v3;
}
Parfait::Point<double> inf::Cell::faceAreaNormal(int face_number) const {
    if (dimension_mode == 2) return faceAreaNormal2D(face_number);
    auto face_points = facePoints(face_number);
    Parfait::Point<double> normal =
        Parfait::Metrics::computeTriangleArea(face_points[0], face_points[1], face_points[2]);
    if (face_points.size() == 4)
        normal +=
            Parfait::Metrics::computeTriangleArea(face_points[2], face_points[3], face_points[0]);
    PARFAIT_ASSERT(face_points.size() <= 4, "Found a face with more than 4 points");
    return normal;
}
int inf::Cell::dimension() const {
    switch (int(cell_type)) {
        case MeshInterface::NODE:
            return 0;
        case MeshInterface::BAR_2:
            return 1;
        case MeshInterface::TRI_3:
            return 2;
        case MeshInterface::QUAD_4:
            return 2;
        case MeshInterface::TETRA_4:
            return 3;
        case MeshInterface::PYRA_5:
            return 3;
        case MeshInterface::PENTA_6:
            return 3;
        case MeshInterface::HEXA_8:
            return 3;
        default:
            PARFAIT_THROW("dimension not implemented for type: " +
                          MeshInterface::cellTypeString(cell_type));
    }
}
void inf::Cell::visualize(std::string filename) const {
    auto _points = this->points();
    auto _nodes = this->nodes();
    auto _type = this->type();
    int length = _points.size();
    auto get_point = [&](long i, double* p) {
        p[0] = _points[i][0];
        p[1] = _points[i][1];
        p[2] = _points[i][2];
    };
    auto get_type = [&](long _) {
        return Parfait::vtk::Writer::CellType(inf::infinityToVtkCellType(_type));
    };
    auto get_nodes = [&](int _, int* n) {
        for (int i = 0; i < length; i++) {
            n[i] = i;
        }
    };

    {
        Parfait::vtk::Writer writer(filename, length, 1, get_point, get_type, get_nodes);
        writer.visualize();
    }

    {
        Parfait::tecplot::Writer writer(filename, length, 1, get_point, get_type, get_nodes);
        writer.visualize();
    }
}
double inf::Cell::volume() const {
    if (dimension_mode == 2) {
        return area2D();
    }
    auto centroid = averageCenter();
    double v = 0;
    for (int face_id = 0; face_id < faceCount(); face_id++) {
        auto face_points = facePoints(face_id);
        v += Parfait::Metrics::computeTetVolume(
            face_points[0], face_points[2], face_points[1], centroid);
        if (face_points.size() == 4) {
            v += Parfait::Metrics::computeTetVolume(
                face_points[0], face_points[3], face_points[2], centroid);
        }
    }

    return v;
}
std::vector<Parfait::Point<double>> inf::Cell::facePoints2D(int face_number) const {
    std::vector<Parfait::Point<double>> face_points;
    FaceNeighbors::getFace2D(cell_type, points(), face_number, face_points);
    return face_points;
}
double inf::Cell::area2D() const {
    if (cell_points.size() < 3) return 0;
    Parfait::Point<double> normal =
        Parfait::Metrics::computeTriangleArea(cell_points[0], cell_points[1], cell_points[2]);
    if (cell_points.size() == 4)
        normal +=
            Parfait::Metrics::computeTriangleArea(cell_points[2], cell_points[3], cell_points[0]);
    PARFAIT_ASSERT(cell_points.size() <= 4,
                   "can't compute area of cell with more points than a quad");
    return normal.magnitude();
}
bool inf::Cell::contains(Parfait::Point<double> p) {
    auto e = Parfait::ExtentBuilder::build(points());
    if (not e.intersects(p)) return false;

    for (int f = 0; f < faceCount(); f++) {
        auto face_normal = faceAreaNormal(f) * (-1.0);  // faces point out of cells typically
        auto face_centroid = faceAverageCenter(f);
        Parfait::Point<double> v = p - face_centroid;
        face_normal.normalize();
        v.normalize();
        if (v.dot(face_normal) < 0.0) {
            return false;
        }
    }
    return true;
}

int inf::Cell::boundingEntityCount() const {
    switch (cell_type) {
        case MeshInterface::BAR_2:
            return 2;
        case MeshInterface::TRI_3:
        case MeshInterface::QUAD_4:
            return faceCount2D();
        case MeshInterface::TETRA_4:
        case MeshInterface::PYRA_5:
        case MeshInterface::PENTA_6:
        case MeshInterface::HEXA_8:
            return faceCount();
        default:
            PARFAIT_THROW("Don't know how to get bounds of a cell of type" +
                          MeshInterface::cellTypeString(cell_type));
            return {};
    }
}
std::vector<int> inf::Cell::nodesInBoundingEntity(int id) const {
    switch (cell_type) {
        case MeshInterface::BAR_2:
            return {cell_nodes[id]};
        case MeshInterface::TRI_3:
        case MeshInterface::QUAD_4:
            return faceNodes(id, cell_type, cell_nodes, 2);
        case MeshInterface::TETRA_4:
        case MeshInterface::PYRA_5:
        case MeshInterface::PENTA_6:
        case MeshInterface::HEXA_8:
            return faceNodes(id, cell_type, cell_nodes, 3);
        default:
            PARFAIT_THROW("Don't know how to get bounds of a cell of type" +
                          MeshInterface::cellTypeString(cell_type));
            return {};
    }
}
