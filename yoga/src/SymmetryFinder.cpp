#include "SymmetryFinder.h"

namespace YOGA {
SymmetryFinder::SymmetryFinder(MessagePasser mp, const YogaMesh& m) : mp(mp), mesh(m) {}

std::vector<SymmetryPlane> SymmetryFinder::findSymmetryPlanes() {
    std::vector<SymmetryPlane> planes;
    auto symmetry_flags = flagComponentsWithSymmetry();
    auto plane_positions = locatePlanes(symmetry_flags);
    for (int i = 0; i < long(symmetry_flags.size()); i++)
        if (symmetry_flags[i] > -1) planes.push_back(SymmetryPlane(i, symmetry_flags[i], plane_positions[i]));
    return planes;
}

std::vector<double> SymmetryFinder::locatePlanes(std::vector<int> symmetry_flags) {
    std::vector<double> symmetry_positions(symmetry_flags.size(), 0);
    for (int i = 0; i < long(symmetry_flags.size()); i++)
        if (-1 < symmetry_flags[i]) symmetry_positions[i] = calcPlanePosition(i, symmetry_flags[i]);
    return symmetry_positions;
}

double SymmetryFinder::calcPlanePosition(int component_id, int symmetry_flag) {
    using namespace YOGA;

    double position = std::numeric_limits<double>::lowest();
    for (int i = 0; i < mesh.numberOfBoundaryFaces(); i++) {
        if (BoundaryConditions::SymmetryX == mesh.getBoundaryCondition(i))
            position = std::max(position, getPosition(i, 0));
        else if (BoundaryConditions::SymmetryY == mesh.getBoundaryCondition(i))
            position = std::max(position, getPosition(i, 1));
        else if (BoundaryConditions::SymmetryZ == mesh.getBoundaryCondition(i))
            position = std::max(position, getPosition(i, 2));
    }
    position = mp.ParallelMax(position, 0);
    mp.Broadcast(position, 0);
    return position;
}

std::vector<int> SymmetryFinder::flagComponentsWithSymmetry() {
    using namespace YOGA;
    int n = countComponents();
    std::vector<int> has_symmetry(n, -1);
    for (int i = 0; i < mesh.numberOfBoundaryFaces(); i++) {
        int imesh = boundaryFaceImesh(i);
        if (BoundaryConditions::SymmetryX == mesh.getBoundaryCondition(i))
            has_symmetry[imesh] = 0;
        else if (BoundaryConditions::SymmetryY == mesh.getBoundaryCondition(i))
            has_symmetry[imesh] = 1;
        else if (BoundaryConditions::SymmetryZ == mesh.getBoundaryCondition(i))
            has_symmetry[imesh] = 2;
    }
    has_symmetry = mp.ParallelMax(has_symmetry, 0);
    mp.Broadcast(has_symmetry, 0);
    return has_symmetry;
}

int SymmetryFinder::countComponents() {
    int highest_component_id = 0;
    for (int i = 0; i < mesh.nodeCount(); i++)
        highest_component_id = std::max(highest_component_id, mesh.getAssociatedComponentId(i));
    highest_component_id = mp.ParallelMax(highest_component_id, 0);
    mp.Broadcast(highest_component_id, 0);
    return highest_component_id + 1;
}

int SymmetryFinder::boundaryFaceImesh(int id) {
    auto face = mesh.getNodesInBoundaryFace(id);
    return mesh.getAssociatedComponentId(face[0]);
}

double SymmetryFinder::getPosition(int boundary_face_id, int dimension) {
    auto face = mesh.getNodesInBoundaryFace(boundary_face_id);
    auto p = mesh.getNode<double>(face[0]);
    return p[dimension];
}
}