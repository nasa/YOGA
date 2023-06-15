#include <parfait/CellContainmentChecker.h>
#include "AdtDonorFinder.h"
#include "InterpolationTools.h"

namespace YOGA {

template <int N>
std::vector<double> getVertexDistances(std::vector<TransferNode>& nodes, std::array<int, N>& cell) {
    std::vector<double> distances;
    for (auto& id : cell) distances.push_back(nodes[id].distanceToWall);
    return distances;
}

template <int N>
auto getXyzForCell(std::array<int, N>& cell, WorkVoxel& workVoxel) {
    std::array<Parfait::Point<double>, N> xyz;
    for (int i = 0; i < N; ++i) xyz[i] = workVoxel.nodes[cell[i]].xyz;
    return xyz;
}

AdtDonorFinder::AdtDonorFinder(WorkVoxel& vox)
    : workVoxel(vox), voxel_extent(getExtentForVoxel(vox)), component_ids(getComponentIdsForVoxel(vox)) {
    for (size_t i = 0; i < component_ids.size(); i++) adts.push_back(std::make_shared<Parfait::Adt3DExtent>(voxel_extent));

    for (size_t i = 0; i < component_ids.size(); i++) {
        fillAdt(*adts[i], component_ids[i]);
    }
}

void AdtDonorFinder::fillAdt(Parfait::Adt3DExtent& adt, int component) {
    for (size_t i = 0; i < workVoxel.tets.size(); ++i) {
        if (component == getCellComponent(i)) {
            auto cellExtent = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
            auto tet = workVoxel.tets[i];
            for (auto nodeId : tet.nodeIds)
                Parfait::ExtentBuilder::add(cellExtent, workVoxel.nodes[nodeId].xyz);
            adt.store(i, cellExtent);
        }
    }
    for (size_t i = 0; i < workVoxel.pyramids.size(); ++i) {
        if (component == getCellComponent(i + workVoxel.tets.size())) {
            auto cellExtent = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
            auto pyramid = workVoxel.pyramids[i];
            for (auto nodeId : pyramid.nodeIds)
                Parfait::ExtentBuilder::add(cellExtent, workVoxel.nodes[nodeId].xyz);
            adt.store(i + workVoxel.tets.size(), cellExtent);
        }
    }
    for (size_t i = 0; i < workVoxel.prisms.size(); ++i) {
        if (component == getCellComponent(i + workVoxel.tets.size() + workVoxel.pyramids.size())) {
            auto cellExtent = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
            auto prism = workVoxel.prisms[i];
            for (auto nodeId : prism.nodeIds)
                Parfait::ExtentBuilder::add(cellExtent, workVoxel.nodes[nodeId].xyz);
            adt.store(i + workVoxel.tets.size() + workVoxel.pyramids.size(), cellExtent);
        }
    }
    for (size_t i = 0; i < workVoxel.hexs.size(); ++i) {
        if (component ==
            getCellComponent(i + workVoxel.tets.size() + workVoxel.pyramids.size() + workVoxel.prisms.size())) {
            auto cellExtent = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
            auto hex = workVoxel.hexs[i];
            for (auto nodeId : hex.nodeIds)
                Parfait::ExtentBuilder::add(cellExtent, workVoxel.nodes[nodeId].xyz);
            adt.store(i + workVoxel.tets.size() + workVoxel.pyramids.size() + workVoxel.prisms.size(), cellExtent);
        }
    }
}

std::vector<int> AdtDonorFinder::getComponentIdsForVoxel(const WorkVoxel& w) {
    std::set<int> unique_ids;
    for (auto& node : w.nodes) unique_ids.insert(node.associatedComponentId);
    return std::vector<int>(unique_ids.begin(), unique_ids.end());
}

int AdtDonorFinder::getCellSize(int cell_id) const {
    if (cell_id < long(workVoxel.tets.size())) return 4;
    cell_id -= workVoxel.tets.size();
    if (cell_id < long(workVoxel.pyramids.size())) return 5;
    cell_id -= workVoxel.pyramids.size();
    if (cell_id < long(workVoxel.prisms.size())) return 6;
    return 8;
}

int AdtDonorFinder::getIdInType(int cell_id) const {
    if (cell_id < long(workVoxel.tets.size())) return cell_id;
    cell_id -= workVoxel.tets.size();
    if (cell_id < long(workVoxel.pyramids.size())) return cell_id;
    cell_id -= workVoxel.pyramids.size();
    if (cell_id < long(workVoxel.prisms.size())) return cell_id;
    cell_id -= workVoxel.prisms.size();
    return cell_id;
}

std::vector<CandidateDonor> AdtDonorFinder::findDonors(Parfait::Point<double>& p, int associatedComponent) {
    std::vector<CandidateDonor> donorCandidates;
    std::vector<int> potential_candidates;
    for (size_t i = 0; i < component_ids.size(); i++) {
        int tree_component = component_ids[i];
        if (tree_component != associatedComponent) {
            auto more_candidates = adts[i]->retrieve(Parfait::Extent<double>(p, p));
            potential_candidates.insert(potential_candidates.end(), more_candidates.begin(), more_candidates.end());
        }
    }
    std::vector<int> cell(8);
    for (int cellId : potential_candidates) {
        int component_of_cell = getCellComponent(cellId);
        if (component_of_cell == associatedComponent) continue;
        int cell_size = getCellSize(cellId);
        int id_in_type = getIdInType(cellId);
        if (4 == cell_size) {
            auto& tet = workVoxel.tets[id_in_type];
            auto tetXyz = getXyzForCell<4>(tet.nodeIds, workVoxel);
            if(not workVoxel.is_in_cell(tetXyz.front().data(),4,p.data())) continue;
            auto vertexDistances = getVertexDistances<4>(workVoxel.nodes, tet.nodeIds);
            double interpolatedDistance =
                least_squares_interpolate(4, tetXyz.data()->data(), vertexDistances.data(), p.data());
            donorCandidates.emplace_back(
                CandidateDonor(component_of_cell, tet.cellId, tet.owningRank, interpolatedDistance,CandidateDonor::Tet));
        } else if (5 == cell_size) {
            auto& pyramid = workVoxel.pyramids[id_in_type];
            auto pyramid_xyz = getXyzForCell<5>(pyramid.nodeIds, workVoxel);
            if(not workVoxel.is_in_cell(pyramid_xyz.front().data(),5,p.data())) continue;
            auto vertexDistances = getVertexDistances<5>(workVoxel.nodes, pyramid.nodeIds);
            double interpolatedDistance =
                least_squares_interpolate(5, pyramid_xyz.data()->data(), vertexDistances.data(), p.data());
            donorCandidates.emplace_back(
                CandidateDonor(component_of_cell, pyramid.cellId, pyramid.owningRank, interpolatedDistance,CandidateDonor::Pyramid));
        } else if (6 == cell_size) {
            auto& prism = workVoxel.prisms[id_in_type];
            auto prism_xyz = getXyzForCell<6>(prism.nodeIds, workVoxel);
            if(not workVoxel.is_in_cell(prism_xyz.front().data(),6,p.data())) continue;
            auto vertexDistances = getVertexDistances<6>(workVoxel.nodes, prism.nodeIds);
            double interpolatedDistance =
                least_squares_interpolate(6, prism_xyz.data()->data(), vertexDistances.data(), p.data());
            donorCandidates.emplace_back(
                CandidateDonor(component_of_cell, prism.cellId, prism.owningRank, interpolatedDistance,CandidateDonor::Prism));
        } else if (8 == cell_size) {
            auto& hex = workVoxel.hexs[id_in_type];
            auto hex_xyz = getXyzForCell<8>(hex.nodeIds, workVoxel);
            if(not workVoxel.is_in_cell(hex_xyz.front().data(),8,p.data())) continue;
            auto vertexDistances = getVertexDistances<8>(workVoxel.nodes, hex.nodeIds);
            double interpolatedDistance =
                least_squares_interpolate(8, hex_xyz.data()->data(), vertexDistances.data(), p.data());
            donorCandidates.emplace_back(
                CandidateDonor(component_of_cell, hex.cellId, hex.owningRank, interpolatedDistance,CandidateDonor::Hex));
        }
    }
    return donorCandidates;
}

int AdtDonorFinder::getCellComponent(int cellId) const {
    int cell_size = getCellSize(cellId);
    int id_in_type = getIdInType(cellId);
    if (4 == cell_size) {
        int first_local_node_id = workVoxel.tets[id_in_type].nodeIds[0];
        return workVoxel.nodes[first_local_node_id].associatedComponentId;
    }
    if (5 == cell_size) {
        int first_local_node_id = workVoxel.pyramids[id_in_type].nodeIds[0];
        return workVoxel.nodes[first_local_node_id].associatedComponentId;
    }
    if (6 == cell_size) {
        int first_local_node_id = workVoxel.prisms[id_in_type].nodeIds[0];
        return workVoxel.nodes[first_local_node_id].associatedComponentId;
    }
    if (8 == cell_size) {
        int first_local_node_id = workVoxel.hexs[id_in_type].nodeIds[0];
        return workVoxel.nodes[first_local_node_id].associatedComponentId;
    }
    printf("Yoga: invalid cell id/type\n");
    throw std::logic_error("invalid id/type");
}

Parfait::Extent<double> AdtDonorFinder::getExtentForVoxel(WorkVoxel& workVoxel) {
    auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    for (auto& p : workVoxel.nodes) Parfait::ExtentBuilder::add(e, p.xyz);
    return e;
}

}
