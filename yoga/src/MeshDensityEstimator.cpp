#include "MeshDensityEstimator.h"
#include <Tracer.h>
#include "HoleCuttingTools.h"
namespace YOGA {
std::vector<double> MeshDensityEstimator::tallyNodesContainedByCartCells(MessagePasser mp,
                                                                      const YogaMesh& mesh,
                                                                      const std::vector<Parfait::Extent<double>>& component_extents,
                                                                      Parfait::CartBlock& block) {
    Tracer::begin("local");
    //auto local_tally = tallyNodesLocally(mesh, block,component_extents, mp.Rank());
    auto local_tally = tallyCrossingCellsLocally(mesh, block,component_extents, mp.Rank());
    Tracer::end("local");
    Tracer::begin("parallel sum");
    mp.ElementalSum(local_tally,0);
    Tracer::end("parallel sum");
    return local_tally;
}

std::vector<int> MeshDensityEstimator::tallyNodesLocally(const YogaMesh& mesh,Parfait::CartBlock& block,
    const std::vector<Parfait::Extent<double>>& component_extents,int rank) {
    std::vector<int> nodesPerCell(block.numberOfCells(), 0);
    for (int i = 0; i < mesh.nodeCount(); ++i) {
        if (mesh.nodeOwner(i) != rank) continue;
        auto p = mesh.getNode<double>(i);
        int component = mesh.getAssociatedComponentId(i);
        if(not doesOverlapOtherComponent(component_extents,p,component)) continue;
        if (block.intersects(p)) {
            int containingCellId = block.getIdOfContainingCell(p.data());
            nodesPerCell[containingCellId]++;
        }
    }
    return nodesPerCell;
}

std::vector<double> MeshDensityEstimator::tallyCrossingCellsLocally(const YogaMesh& mesh,
        Parfait::CartBlock& block,
        const std::vector<Parfait::Extent<double>>& component_extents,int rank) {
    std::vector<double> crossing_counts(block.numberOfCells(),0);
    for(int cell_id=0;cell_id<mesh.numberOfCells();cell_id++){
        auto cell = mesh.cell_ptr(cell_id);
        int n = mesh.numberOfNodesInCell(cell_id);
        auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        for(int j=0;j<n;j++){
            Parfait::ExtentBuilder::add(e,mesh.getNode<double>(cell[j]));
        }
        int cell_component = mesh.getAssociatedComponentId(cell[0]);
        if(not doesOverlapOtherComponent(component_extents,e,cell_component)) continue;
        if(e.intersects(block)){
            auto range = block.getRangeOfOverlappingCells(e);
            double factor = 1. / (double(range.hi[0]-range.lo[0])*(range.hi[1]-range.lo[1])*(range.hi[2]-range.lo[2]));
            for(int i=range.lo[0];i<range.hi[0];i++){
                for(int j=range.lo[1];j<range.hi[1];j++){
                    for(int k=range.lo[2];k<range.hi[2];k++){
                        int cart_cell_id = block.convert_ijk_ToCellId(i,j,k);
                        crossing_counts[cart_cell_id] += factor;
                    }
                }
            }
        }
    }
    return crossing_counts;
}

}