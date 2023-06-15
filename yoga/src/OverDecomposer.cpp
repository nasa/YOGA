#include "OverDecomposer.h"
#include <parfait/RecursiveBisection.h>

namespace YOGA {

std::vector<Parfait::Point<double>> OverDecomposer::generateCellCenters(const YogaMesh& mesh,const std::vector<int>& cell_ids){
    std::vector<Parfait::Point<double>> cell_centers(cell_ids.size());
    for (size_t i=0;i<cell_ids.size();i++){
        int cell_id = cell_ids[i];
        auto cell = mesh.cell_ptr(cell_id);
        int n = mesh.numberOfNodesInCell(cell_id);
        auto p = mesh.getNode<double>(cell[0]);
        for(int j=1;j<n;j++)
            p += mesh.getNode<double>(cell[j]);
        p *= 1.0/n;
        cell_centers[i] = p;
    }
    return cell_centers;
}

std::vector<VoxelFragment> OverDecomposer::extractFragments(const YogaMesh& mesh,
        const std::vector<int>& part, const std::vector<int>& owned_cell_ids,
        int n_partitions,
        int rank){
    std::vector<VoxelFragment> fragments;
    Tracer::begin("build vector");
    Tracer::begin("map cells to partitions");
    auto cells_for_partitions = mapCellsToPartitions(n_partitions, part, owned_cell_ids);
    Tracer::end("map cells to partitions");
    auto node_bcs = PartitionInfo::createNodeBcs(mesh);

    for (int i = 0; i < n_partitions; i++) {
        auto& cells = cells_for_partitions[i];
        fragments.emplace_back(VoxelFragment(mesh, node_bcs, cells, rank));
    }
    Tracer::end("build vector");
    return fragments;
}

    bool doesOverlapAnotherComponent(const Parfait::Extent<double>& e,int component,
    const MeshSystemInfo& mesh_system_info){
    for(int i=0;i<mesh_system_info.numberOfComponents();i++){
        if(i==component) continue;
        if(e.intersects(mesh_system_info.getComponentExtent(i))){
            return true;
        }
    }
    return false;
}

std::vector<int> OverDecomposer::identifyOwnedCells(const YogaMesh& mesh,
    const PartitionInfo& info,
    const MeshSystemInfo& mesh_system_info){
    std::vector<int> owned_cells;
    owned_cells.reserve(mesh.numberOfCells());
    for(int i=0;i<mesh.numberOfCells();i++){
        if(info.isCellMine(i)){
            if(doesOverlapAnotherComponent(info.getExtentForCell(i),
                info.getAssociatedComponentIdForCell(i),mesh_system_info)) {
                owned_cells.push_back(i);
            }
        }
    }
    return owned_cells;
}

std::vector<std::vector<int>> OverDecomposer::mapCellsToPartitions(int npart, const std::vector<int>& part,
    const std::vector<int>& owned_cell_ids) {
    std::vector<std::vector<int>> cells_for_partitions(npart);
    for (size_t i = 0; i < part.size(); i++) {
        int n = part[i];
        int cell_id = owned_cell_ids[i];
        cells_for_partitions[n].push_back(cell_id);
    }
    return cells_for_partitions;
}
}