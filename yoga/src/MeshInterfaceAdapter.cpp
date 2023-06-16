#include "MeshInterfaceAdapter.h"
#include <algorithm>
#include <vector>
#include "GlobalIdTranslator.h"
#include "RankTranslator.h"

namespace YOGA {

MeshInterfaceAdapter::MeshInterfaceAdapter(MessagePasser mp,
                                           const inf::MeshInterface& mesh,
                                           int component_id,
                                           const std::vector<YOGA::BoundaryConditions>& boundary_conditions)
    : mp(mp),
      mesh(mesh),
      component_id(component_id),
      node_owning_rank(calcOwningRanksInGlobalComm(extractNodeOwners(mesh))),
      cell_owning_rank(calcOwningRanksInGlobalComm(extractCellOwners(mesh))),
      global_node_ids(createGlobalNodeIds()),
      boundary_conditions(boundary_conditions),
      triToCellId(createTriToCellMap(mesh)),
      quadToCellId(createQuadToCellMap(mesh)),
      volumeCellToFrameworkCellId(createVolumeCellToCellIdMap(mesh)) {}

int MeshInterfaceAdapter::numberOfNodes() const { return mesh.nodeCount(); }

int MeshInterfaceAdapter::numberOfCells() const {
    return mesh.cellCount() - mesh.cellCount(inf::MeshInterface::CellType::BAR_2) -
           mesh.cellCount(inf::MeshInterface::CellType::TRI_3) -
           mesh.cellCount(inf::MeshInterface::CellType::QUAD_4);
}

long MeshInterfaceAdapter::getGlobalNodeId(int id) const { return global_node_ids.at(id); }

std::vector<int> MeshInterfaceAdapter::extractNodeOwners(const inf::MeshInterface& mesh) const {
    std::vector<int> node_owners(mesh.nodeCount());
    for (int i = 0; i < mesh.nodeCount(); i++) node_owners[i] = mesh.nodeOwner(i);
    return node_owners;
}

std::vector<int> MeshInterfaceAdapter::extractCellOwners(const inf::MeshInterface& mesh) const {
    std::vector<int> cell_owners(mesh.cellCount());
    for (int i = 0; i < mesh.cellCount(); i++) cell_owners[i] = mesh.cellOwner(i);
    return cell_owners;
}

void MeshInterfaceAdapter::throwIfBadCellType(int id) const {
    int framework_cell_id = volumeCellToFrameworkCellId.at(id);
    auto type = mesh.cellType(framework_cell_id);
    if (inf::MeshInterface::TETRA_4 == type or inf::MeshInterface::PENTA_6 == type or
        inf::MeshInterface::PYRA_5 == type or inf::MeshInterface::HEXA_8 == type) {
    } else {
        printf("YOGA BAD CELL TYPE: %s\n", inf::MeshInterface::cellTypeString(type).c_str());
        fflush(stdout);
    }
}

int MeshInterfaceAdapter::numberOfNodesInCell(int id) const {
    throwIfBadCellType(id);
    int framework_cell_id = volumeCellToFrameworkCellId[id];
    auto cell_type = mesh.cellType(framework_cell_id);
    return mesh.cellTypeLength(cell_type);
}

std::vector<int> MeshInterfaceAdapter::getNodesInCell(int id) const {
    throwIfBadCellType(id);
    int framework_cell_id = volumeCellToFrameworkCellId[id];

    auto type = mesh.cellType(framework_cell_id);
    int cell_length = inf::MeshInterface::cellTypeLength(type);
    std::vector<int> cell(cell_length);
    mesh.cell(framework_cell_id, cell.data());
    return cell;
}

int MeshInterfaceAdapter::frameworkCellId(int id) const {
    throwIfBadCellType(id);
    return volumeCellToFrameworkCellId.at(id);
}

void MeshInterfaceAdapter::getNodesInCell(int id, int* cell) const {
    throwIfBadCellType(id);
    int framework_cell_id = volumeCellToFrameworkCellId.at(id);
    mesh.cell(framework_cell_id, cell);
}

Parfait::Point<double> MeshInterfaceAdapter::getNode(int id) const {
    return mesh.node(id);
}

Parfait::Point<double> MeshInterfaceAdapter::getPoint(int id) const { return getNode(id); }

int MeshInterfaceAdapter::cellOwner(int cell_id) const {
    throwIfBadCellType(cell_id);
    int framework_cell_id = volumeCellToFrameworkCellId.at(cell_id);
    return cell_owning_rank.at(framework_cell_id);
}

int MeshInterfaceAdapter::getNodeOwner(int id) const { return node_owning_rank[id]; }
int MeshInterfaceAdapter::getAssociatedComponentId(int id) const { return component_id; }

int MeshInterfaceAdapter::numberOfBoundaryFaces() const {
    return mesh.cellCount(inf::MeshInterface::CellType::TRI_3) +
           mesh.cellCount(inf::MeshInterface::CellType::QUAD_4);
}

std::vector<int> MeshInterfaceAdapter::getNodesInBoundaryFace(int face_id) const {
    int cell_id = -1;
    if(face_id < long(triToCellId.size())) {
        cell_id = triToCellId.at(face_id);
    }else{
        cell_id = quadToCellId.at(face_id-triToCellId.size());
    }
    auto cell_type = mesh.cellType(cell_id);
    auto cell_length = mesh.cellTypeLength(cell_type);
    std::vector<int> face(cell_length);
    mesh.cell(cell_id, face.data());
    return face;
}

BoundaryConditions MeshInterfaceAdapter::getBoundaryCondition(int id) const { return boundary_conditions.at(id); }

std::vector<int> MeshInterfaceAdapter::createTriToCellMap(const inf::MeshInterface& mesh) {
    std::vector<int> tri_to_cell;
    for (int i = 0; i < mesh.cellCount(); i++) {
        auto cell_type = mesh.cellType(i);
        if (cell_type == inf::MeshInterface::CellType::TRI_3) tri_to_cell.push_back(i);
    }
    return tri_to_cell;
}

std::vector<int> MeshInterfaceAdapter::createQuadToCellMap(const inf::MeshInterface& mesh) {
    std::vector<int> quad_to_cell;
    for (int i = 0; i < mesh.cellCount(); i++) {
        auto cell_type = mesh.cellType(i);
        if (cell_type == inf::MeshInterface::CellType::QUAD_4) quad_to_cell.push_back(i);
    }
    return quad_to_cell;
}

std::vector<int> MeshInterfaceAdapter::createVolumeCellToCellIdMap(const inf::MeshInterface& mesh) {
    std::vector<int> volume_to_cell;
    for (int i = 0; i < mesh.cellCount(); i++) {
        auto cell_type = mesh.cellType(i);
        if (cell_type == inf::MeshInterface::TETRA_4 or cell_type == inf::MeshInterface::PENTA_6 or
            cell_type == inf::MeshInterface::PYRA_5 or cell_type == inf::MeshInterface::HEXA_8)
            volume_to_cell.push_back(i);
    }
    return volume_to_cell;
}

std::vector<long> MeshInterfaceAdapter::createGlobalNodeIds() {
    auto nodes_per_domain = calcNodesPerDomain();
    GlobalIdTranslator translator(nodes_per_domain);
    std::vector<long> gids(mesh.nodeCount());
    for (int i = 0; i < mesh.nodeCount(); i++) {
        gids[i] = translator.globalId(mesh.globalNodeId(i), component_id);
    }
    return gids;
}

std::vector<int> MeshInterfaceAdapter::calcOwningRanksInGlobalComm(const std::vector<int>& owner_in_comm) {
    auto ranks_per = calcRanksPerDomain();
    RankTranslator<int> translator(ranks_per);
    std::vector<int> owner_in_global_comm(owner_in_comm.size());
    for (size_t i = 0; i < owner_in_comm.size(); i++)
        owner_in_global_comm[i] = translator.globalRank(owner_in_comm[i], component_id);
    return owner_in_global_comm;
}

std::vector<long> MeshInterfaceAdapter::calcNodesPerDomain() {
    int ndomains = mp.ParallelMax(component_id) + 1;
    std::vector<long> nodes_per_domain(ndomains);
    long my_max_node_id = calcMaxNodeId();

    for (int i = 0; i < ndomains; i++) {
        if (i == component_id)
            nodes_per_domain[i] = mp.ParallelMax(my_max_node_id) + 1;
        else
            nodes_per_domain[i] = mp.ParallelMax(long(0)) + 1;
    }
    return nodes_per_domain;
}

std::vector<int> MeshInterfaceAdapter::calcRanksPerDomain() {
    int n = mp.ParallelMax(component_id) + 1;
    std::vector<int> ranks_per(n);
    ranks_per[component_id]++;
    for (int i = 0; i < n; i++) {
        ranks_per[i] = mp.ParallelSum(ranks_per[i]);
    }
    return ranks_per;
}

long MeshInterfaceAdapter::calcMaxNodeId() {
    long max_id = 0;
    for (int i = 0; i < mesh.nodeCount(); i++) max_id = std::max(max_id, mesh.globalNodeId(i));
    return max_id;
}

}
