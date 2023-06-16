#include "PartitionInfo.h"
#include <Tracer.h>

namespace YOGA {

PartitionInfo::PartitionInfo(const YogaMesh& mesh, int rank)
    : boundaryConditionsForNodes(createNodeBcs(mesh)),
      boundaryConditionsForFaces(createFaceBcs(mesh)),
      is_donor_candidate(determineDonorCandidacy(mesh, boundaryConditionsForNodes)) {
    Tracer::begin("face component list");
    buildFaceComponentList(mesh);
    Tracer::end("face component list");
    Tracer::begin("face extents");
    buildFaceExtents(mesh);
    Tracer::end("face extents");
    Tracer::begin("component list");
    buildCellComponentList(mesh);
    Tracer::end("component list");
    Tracer::begin("cell extents");
    buildCellExtents(mesh);
    Tracer::end("cell extents");
    // TODO: use function to determine if part of solid surface instead of checking number directly

    Tracer::begin("check for solid surfaces");
    for (int i = 0; i < mesh.numberOfBoundaryFaces(); i++)
        if (Solid == mesh.getBoundaryCondition(i)) addFaceExtentToBodyExtent(mesh, i);
    Tracer::end("check for solid surfaces");

    Tracer::begin("component extent");
    for (int i = 0; i < mesh.nodeCount(); i++) addNodeToComponentExtent(mesh, i);
    Tracer::end("component extent");
    Tracer::begin("cell ownership");
    createCellOwnership(mesh, rank);
    Tracer::end("cell ownership");
}

int PartitionInfo::numberOfBodies() const { return body_extents.size(); }
int PartitionInfo::numberOfComponentMeshes() const { return component_extents.size(); }
bool PartitionInfo::containsBody(int id) const { return 1 == body_extents.count(id); }
bool PartitionInfo::containsComponentMesh(int id) const { return 1 == component_extents.count(id); }

Parfait::Extent<double> PartitionInfo::getExtentForBody(int i) const {
    throwIfBodyNotFound(i);
    return body_extents.at(i);
}

Parfait::Extent<double> PartitionInfo::getExtentForComponent(int i) const {
    throwIfComponentNotFound(i);
    return component_extents.at(i);
}

int PartitionInfo::getAssociatedComponentIdForFace(int i) const { return face_associated_component_id[i]; }
Parfait::Extent<double> PartitionInfo::getExtentForFace(int i) const {
    if (i > long(face_extents.size())) throw std::domain_error("invalid face request");
    return face_extents[i];
}
int PartitionInfo::getAssociatedComponentIdForCell(int i) const { return cell_associated_component_id[i]; }
const Parfait::Extent<double>& PartitionInfo::getExtentForCell(int i) const { return cell_extents[i]; }

bool PartitionInfo::isCellMine(int i) const { return i_own_cell[i]; }

std::vector<int> PartitionInfo::getBodyIds() const {
    std::vector<int> body_ids;
    for (auto& item : body_extents) body_ids.push_back(item.first);
    return body_ids;
}

std::vector<int> PartitionInfo::getComponentIds() const {
    std::vector<int> component_ids;
    for (auto& item : component_extents) component_ids.push_back(item.first);
    return component_ids;
}

std::vector<BoundaryConditions> PartitionInfo::createFaceBcs(const YogaMesh& mesh) {
    std::vector<BoundaryConditions> bcs;
    for (int i = 0; i < mesh.numberOfBoundaryFaces(); ++i) {
        bcs.push_back(mesh.getBoundaryCondition(i));
    }
    return bcs;
}

std::vector<BoundaryConditions> PartitionInfo::createNodeBcs(const YogaMesh& mesh) {
    std::vector<BoundaryConditions> bcs(mesh.nodeCount(), BoundaryConditions::NotABoundary);
    for (int i = 0; i < mesh.numberOfBoundaryFaces(); ++i)
        for (auto& id : mesh.getNodesInBoundaryFace(i)) bcs[id] = mesh.getBoundaryCondition(i);
    return bcs;
}

void PartitionInfo::createCellOwnership(const YogaMesh& mesh, int rank) {
    i_own_cell.assign(mesh.numberOfCells(), false);
    std::vector<int> cell;
    for (int i = 0; i < mesh.numberOfCells(); ++i) {
        long minGlobalId = std::numeric_limits<long>::max();
        cell.resize(mesh.numberOfNodesInCell(i));
        mesh.getNodesInCell(i, cell.data());
        for (int localId : cell) {
            if (minGlobalId > mesh.globalNodeId(localId)) {
                minGlobalId = mesh.globalNodeId(localId);
                if (rank == mesh.nodeOwner(localId))
                    i_own_cell[i] = true;
                else
                    i_own_cell[i] = false;
            }
        }
    }
}

void PartitionInfo::addFaceExtentToBodyExtent(const YogaMesh& mesh, int face_id) {
    int body_id = face_associated_component_id[face_id];
    auto e = face_extents[face_id];
    if (1 == body_extents.count(body_id))
        Parfait::ExtentBuilder::add(body_extents[body_id], e);
    else
        body_extents[body_id] = e;
}

void PartitionInfo::addNodeToComponentExtent(const YogaMesh& mesh, int i) {
    int id = mesh.getAssociatedComponentId(i);
    auto p = mesh.getNode<double>(i);
    if (1 == component_extents.count(id))
        Parfait::ExtentBuilder::add(component_extents[id], p);
    else
        component_extents[id] = Parfait::Extent<double>{p, p};
}

void PartitionInfo::buildFaceComponentList(const YogaMesh& mesh) {
    face_associated_component_id.assign(mesh.numberOfBoundaryFaces(), 0);
    for (int i = 0; i < mesh.numberOfBoundaryFaces(); i++) {
        auto node_ids = mesh.getNodesInBoundaryFace(i);
        if(node_ids.front() < 0 or node_ids.front() > mesh.nodeCount()){
            throw std::domain_error("Bad node id in face: "+std::to_string(node_ids.front()));
        }
        face_associated_component_id[i] = mesh.getAssociatedComponentId(node_ids[0]);
    }
}

void PartitionInfo::buildFaceExtents(const YogaMesh& mesh) {
    Parfait::Extent<double> empty_extent;
    face_extents.assign(mesh.numberOfBoundaryFaces(), empty_extent);
    for (int i = 0; i < mesh.numberOfBoundaryFaces(); i++) {
        face_extents[i] = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        for(int node_id : mesh.getNodesInBoundaryFace(i))
            Parfait::ExtentBuilder::add(face_extents[i], mesh.getNode<double>(node_id));
    }
}

void PartitionInfo::buildCellComponentList(const YogaMesh& mesh) {
    cell_associated_component_id.assign(mesh.numberOfCells(), 0);
    std::vector<int> cell(8);
    for (int i = 0; i < mesh.numberOfCells(); i++) {
        cell.resize(mesh.numberOfNodesInCell(i));
        mesh.getNodesInCell(i, cell.data());
        cell_associated_component_id[i] = mesh.getAssociatedComponentId(cell[0]);
    }
}

Parfait::Extent<double> createExtent(const YogaMesh& mesh, const std::vector<int>& cell) {
    auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    for (int node_id : cell) {
        auto xyz = mesh.getNode<double>(node_id);
        Parfait::ExtentBuilder::add(e, Parfait::Point<double>(xyz[0], xyz[1], xyz[2]));
    }
    return e;
}

void PartitionInfo::buildCellExtents(const YogaMesh& mesh) {
    std::vector<int> cell(8);
    for (int i = 0; i < mesh.numberOfCells(); i++) {
        cell.resize(mesh.numberOfNodesInCell(i));
        mesh.getNodesInCell(i, cell.data());
        cell_extents.emplace_back(createExtent(mesh, cell));
    }
}

void PartitionInfo::throwIfBodyNotFound(int i) const {
    if (0 == body_extents.count(i)) throw std::domain_error("body not found (" + std::to_string(i) + ")");
}

void PartitionInfo::throwIfComponentNotFound(int i) const {
    if (0 == component_extents.count(i)) throw std::domain_error("component not found (" + std::to_string(i) + ")");
}

std::vector<bool> PartitionInfo::determineDonorCandidacy(const YogaMesh& mesh,
                                                         const std::vector<YOGA::BoundaryConditions>& node_bcs) {
    std::vector<bool> is_candidate(mesh.numberOfCells(), true);
    std::vector<bool> is_node_flagged(mesh.nodeCount(), false);
    std::vector<int> cell;
    // mark cells that contain nodes on interpolation boundaries
    for (int i = 0; i < mesh.numberOfCells(); i++) {
        int n = mesh.numberOfNodesInCell(i);
        cell.resize(n);
        mesh.getNodesInCell(i, cell.data());
        for (int node_id : cell)
            if (node_bcs[node_id] == YOGA::BoundaryConditions::Interpolation) is_candidate[i] = false;
        if (not is_candidate[i])
            for (int node_id : cell) is_node_flagged[node_id] = true;
    }
    // mark neighbors of those cells as well
    for (int i = 0; i < mesh.numberOfCells(); i++) {
        int n = mesh.numberOfNodesInCell(i);
        cell.resize(n);
        mesh.getNodesInCell(i, cell.data());
        for (int node_id : cell)
            if (is_node_flagged[node_id]) is_candidate[i] = false;
    }

    return is_candidate;
}

bool PartitionInfo::isDonorCandidate(int i) const { return is_donor_candidate[i]; }

}
