#include "SurfaceElementWinding.h"
#include <parfait/Throw.h>

namespace SurfaceElementWinding {
Parfait::Point<double> computeTriangleArea(const Parfait::Point<double>& a,
                                           const Parfait::Point<double>& b,
                                           const Parfait::Point<double>& c) {
    auto v1 = b - a;
    auto v2 = c - a;
    return 0.5 * Parfait::Point<double>::cross(v1, v2);
}
Parfait::Point<double> average(const std::vector<Parfait::Point<double>>& points) {
    Parfait::Point<double> avg = {0, 0, 0};
    for (auto& p : points) avg += p;
    return avg / (double)points.size();
}
Parfait::Point<double> computeCellCenter(const inf::TinfMesh& mesh, int cell_id) {
    auto type = mesh.cellType(cell_id);
    int cell_length = mesh.cellTypeLength(type);
    std::vector<int> cell(cell_length);
    mesh.cell(cell_id, cell.data());
    Parfait::Point<double> center = {0, 0, 0};
    for (int i = 0; i < cell_length; i++) {
        auto p = mesh.node(cell[i]);
        center += p;
    }
    return center / (double)cell_length;
}
bool isTriPointingOut(const inf::TinfMesh& mesh, const int* surface, int volume_neighbor) {
    auto a = mesh.node(surface[0]);
    auto b = mesh.node(surface[1]);
    auto c = mesh.node(surface[2]);
    auto surface_area = computeTriangleArea(a, b, c);
    auto surface_center = average({a, b, c});
    auto neighbor_center = computeCellCenter(mesh, volume_neighbor);
    auto out_direction = surface_center - neighbor_center;
    out_direction.normalize();
    surface_area.normalize();
    return (Parfait::Point<double>::dot(out_direction, surface_area) > 0);
}
bool isQuadPointingOut(const inf::TinfMesh& mesh, const int* surface, int volume_neighbor) {
    auto a = mesh.node(surface[0]);
    auto b = mesh.node(surface[1]);
    auto c = mesh.node(surface[2]);
    auto d = mesh.node(surface[3]);
    auto surface_area = computeTriangleArea(a, b, c) + computeTriangleArea(a, c, d);
    auto surface_center = average({a, b, c, d});
    auto neighbor_center = computeCellCenter(mesh, volume_neighbor);
    auto out_direction = surface_center - neighbor_center;
    out_direction.normalize();
    surface_area.normalize();
    return (Parfait::Point<double>::dot(out_direction, surface_area) > 0);
}
void rewindTriangle(int* triangle) { std::swap(triangle[1], triangle[2]); }
void rewindQuad(int* quad) {
    int d = quad[0];
    int c = quad[1];
    int b = quad[2];
    int a = quad[3];
    quad[0] = a;
    quad[1] = b;
    quad[2] = c;
    quad[3] = d;
}
bool areSurfaceElementsWoundOut(const inf::TinfMesh& mesh,
                                const std::vector<int>& surface_neighbors) {
    int surface_index = 0;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        auto cell_type = mesh.cellType(cell_id);
        if (mesh.is2DCellType(cell_type)) {
            int cell_length = mesh.cellTypeLength(cell_type);
            std::vector<int> cell(cell_length);
            mesh.cell(cell_id, cell.data());
            int neighbor = surface_neighbors[surface_index++];
            if (cell_type == inf::MeshInterface::TRI_3) {
                if (not isTriPointingOut(mesh, cell.data(), neighbor)) return false;
            } else if (cell_type == inf::MeshInterface::QUAD_4) {
                if (not isQuadPointingOut(mesh, cell.data(), neighbor)) return false;
            } else {
                PARFAIT_THROW("Implement P2");
            }
        }
    }
    return true;
}
void windAllSurfaceElementsOut(MessagePasser mp,
                               inf::TinfMesh& mesh,
                               const std::vector<int>& surface_neighbors) {
    int surface_index = 0;
    std::vector<int> cell;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        auto type = mesh.cellType(cell_id);
        if (not mesh.is2DCellType(type)) continue;

        mesh.cell(cell_id, cell);
        int neighbor = surface_neighbors[surface_index++];
        if (inf::MeshInterface::TRI_3 == type) {
            if (not isTriPointingOut(mesh, cell.data(), neighbor)) {
                rewindTriangle(cell.data());
                mesh.setCell(cell_id, cell);
            }
        } else if (inf::MeshInterface::QUAD_4 == type) {
            if (not isQuadPointingOut(mesh, cell.data(), neighbor)) {
                rewindQuad(cell.data());
                mesh.setCell(cell_id, cell);
            }
        }
    }
}

}