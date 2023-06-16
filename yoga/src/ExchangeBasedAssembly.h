#pragma once

#include "InterpolationTools.h"
#include "VoxelFragment.h"
namespace YOGA{

class FragmentDonorFinder{
  public:
    FragmentDonorFinder(const std::map<int,VoxelFragment>& frags_from_ranks,
                        std::function<bool(double*, int, double*)> is_in_cell)
    :fragments_from_ranks(frags_from_ranks),
    extent(Parfait::ExtentBuilder::createEmptyBuildableExtent<double>()),
    is_in_cell(is_in_cell){
        for(auto& pair:fragments_from_ranks){
            int rank = pair.first;
            auto& frag = pair.second;
            auto e = calcFragmentExtent(frag);
            Parfait::ExtentBuilder::add(extent,e);
            std::set<int> component_ids;
            for(auto& node:frag.transferNodes) component_ids.insert(node.associatedComponentId);
            for(int component:component_ids){
                adts[rank][component] = std::make_shared<Parfait::Adt3DExtent>(e);
                auto& adt = *adts[rank][component];
                addCellsToAdt(adt,frag,component);
            }
        }
    }

    void clear(){
        adts.clear();
    }

    std::vector<Receptor> generateCandidateReceptors(const std::vector<TransferNode>& query_pts){
        std::vector<Receptor> candidate_receptors;
        std::vector<int> donor_ids;
        for(auto& query_pt:query_pts){
            auto& p = query_pt.xyz;
            int query_component = query_pt.associatedComponentId;
            Receptor receptor;
            receptor.globalId = query_pt.globalId;
            receptor.owner = query_pt.owningRank;
            receptor.distance = query_pt.distanceToWall;
            for(auto& pair:adts){
                int fragment_index = pair.first;
                auto& frag = fragments_from_ranks.at(fragment_index);
                for(auto& pair2:pair.second) {
                    int adt_component = pair2.first;
                    if(adt_component != query_component) {
                        auto& adt = *pair2.second;
                        adt.retrieve({p, p}, donor_ids);
                        removeNonContainingDonors(fragment_index, p, donor_ids);
                        for(int id:donor_ids){
                            int cell_size,index_in_type;
                            getCellSizeAndIndex(frag,id,cell_size,index_in_type);
                            int local_cell_id = getLocalCellId(frag,cell_size,index_in_type);
                            int donor_owner = getCellOwner(frag,cell_size,index_in_type);
                            double donor_distance = calcInterpolatedDistance(frag,p,id);
                            CandidateDonor::CellType cell_type = cellType(frag,cell_size);
                            receptor.candidateDonors.emplace_back(CandidateDonor(
                                adt_component, local_cell_id, donor_owner, donor_distance, cell_type));
                        }
                    }
                    donor_ids.clear();
                }
            }
            if(receptor.candidateDonors.size() > 0) {
                candidate_receptors.emplace_back(receptor);
            }
        }
        return candidate_receptors;
    }

    Parfait::Extent<double> getExtent() const {return extent;}

  private:
    const std::map<int,VoxelFragment>& fragments_from_ranks;
    Parfait::Extent<double> extent;
    std::function<bool(double*, int, double*)> is_in_cell;
    std::map<int,std::map<int,std::shared_ptr<Parfait::Adt3DExtent>>> adts;

    Parfait::Extent<double> calcFragmentExtent(const VoxelFragment& frag){
        auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        for(auto& node:frag.transferNodes) Parfait::ExtentBuilder::add(e,node.xyz);
        return e;
    }

    void removeNonContainingDonors(int fragment_index,const Parfait::Point<double>& p,std::vector<int>& donor_ids){
        std::set<int> ids_to_remove;
        auto& frag = fragments_from_ranks.at(fragment_index);
        std::vector<Parfait::Point<double>> cell;
        for(int id:donor_ids){
            int n;
            const int* ptr;
            getCellSizeAndPointer(frag,id,n,ptr);
            cell.resize(n);
            fillCell(frag,cell,ptr,n);
            if(not is_in_cell(cell.front().data(),n,(double*)p.data())){
                ids_to_remove.insert(id);
            }
        }
        auto end = std::remove_if(donor_ids.begin(),donor_ids.end(),[&](int id){return ids_to_remove.count(id)==1;});
        donor_ids.resize(std::distance(donor_ids.begin(),end));
    }

    void fillCell(const VoxelFragment& frag,std::vector<Parfait::Point<double>>& cell,const int* ptr,int n) const{
        for(int i=0;i<n;i++){
            int node_id = ptr[i];
            cell[i] = frag.transferNodes[node_id].xyz;
        }
    }

    int getLocalCellId(const VoxelFragment& frag,int n,int index_in_type){
        if(4 == n) return frag.transferTets[index_in_type].cellId;
        if(5 == n) return frag.transferPyramids[index_in_type].cellId;
        if(6 == n) return frag.transferPrisms[index_in_type].cellId;
        return frag.transferHexs[index_in_type].cellId;
    }

    int getCellOwner(const VoxelFragment& frag,int n,int index_in_type){
        if(4 == n) return frag.transferTets[index_in_type].owningRank;
        if(5 == n) return frag.transferPyramids[index_in_type].owningRank;
        if(6 == n) return frag.transferPrisms[index_in_type].owningRank;
        return frag.transferHexs[index_in_type].owningRank;
    }

    CandidateDonor::CellType cellType(const VoxelFragment& frag,int n) const {
        if(n == 4) return CandidateDonor::Tet;
        if(n == 5) return CandidateDonor::Pyramid;
        if(n == 6) return CandidateDonor::Prism;
        return CandidateDonor::Hex;
    }

    double calcInterpolatedDistance(const VoxelFragment& frag,const Parfait::Point<double>& p,
        int id){
        int n;
        const int* ptr;
        getCellSizeAndPointer(frag,id,n,ptr);
        std::vector<Parfait::Point<double>> cell(n);
        fillCell(frag,cell,ptr,n);
        std::vector<double> vertex_distances(n);
        for(int i=0;i<n;i++){
            int vertex_node_id = ptr[i];
            vertex_distances[i] = frag.transferNodes[vertex_node_id].distanceToWall;
        }
        return least_squares_interpolate(n,cell.front().data(),vertex_distances.data(),p.data());
    }

    void getCellSizeAndPointer(const VoxelFragment& fragment,int id, int& n,const int*& ptr){
       int index;
       getCellSizeAndIndex(fragment,id,n,index);
       if(4 == n) ptr = fragment.transferTets[index].nodeIds.data();
       else if(5 == n) ptr = fragment.transferPyramids[index].nodeIds.data();
       else if(6 == n) ptr = fragment.transferPrisms[index].nodeIds.data();
       else ptr = fragment.transferHexs[index].nodeIds.data();
    }

    void getCellSizeAndIndex(const VoxelFragment& frag,int id,int& n,int& index) const {
        index = id;
        if(index < long(frag.transferTets.size())){
            n = 4;
            return;
        }
        index -= frag.transferTets.size();
        if(index < long(frag.transferPyramids.size())){
            n = 5;
            return;
        }
        index -= frag.transferPyramids.size();
        if(index < long(frag.transferPrisms.size())){
            n = 6;
            return;
        }
        index -= frag.transferPrisms.size();
        if(index < long(frag.transferHexs.size())){
            n = 8;
            return;
        }
        throw std::logic_error("Yoga: can't determine donor cell type in VoxelFragment");
    }

    template<typename Cell>
    int componentOfCell(const VoxelFragment& frag,const Cell& cell) const {
        return frag.transferNodes[cell.nodeIds.front()].associatedComponentId;
    }

    void addCellsToAdt(Parfait::Adt3DExtent& adt,const VoxelFragment& frag,const int component_id){
        int local_cell_id = 0;
        for(auto& tet:frag.transferTets){
            if(component_id == componentOfCell(frag,tet)) {
                adt.store(local_cell_id, createExtentFromCell(frag, tet.nodeIds.data(), 4));
            }
            local_cell_id++;
        }
        for(auto& pyramid:frag.transferPyramids){
            if(component_id == componentOfCell(frag,pyramid)) {
                adt.store(local_cell_id, createExtentFromCell(frag, pyramid.nodeIds.data(), 5));
            }
            local_cell_id++;
        }
        for(auto& prism:frag.transferPrisms){
            if(component_id == componentOfCell(frag,prism)) {
                adt.store(local_cell_id, createExtentFromCell(frag, prism.nodeIds.data(), 6));
            }
            local_cell_id++;
        }
        for(auto& hex:frag.transferHexs){
            if(component_id == componentOfCell(frag,hex)) {
                adt.store(local_cell_id, createExtentFromCell(frag, hex.nodeIds.data(), 8));
            }
            local_cell_id++;
        }
    }

    Parfait::Extent<double> createExtentFromCell(const VoxelFragment& frag,
            const int* cell,int n){
        auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        for(int i=0;i<n;i++) {
            auto vertex = frag.transferNodes[cell[i]].xyz;
            Parfait::ExtentBuilder::add(e,vertex);
        }
        return e;
    }

    void fillNeighbors(std::vector<std::set<int>>& n2n,const int* cell,int cell_size){
        for(int i=0;i<cell_size;i++){
            for(int j=i+1;j<cell_size;j++){
                int left = cell[i];
                int right = cell[j];
                n2n[left].insert(right);
                n2n[right].insert(left);
            }
        }
    }

    std::vector<std::set<int>> createNodeNeighbors(const VoxelFragment& fragment){
        Tracer::begin("n2n");
        std::vector<std::set<int>> n2n(fragment.transferNodes.size());
        for(const auto & cell : fragment.transferTets){
            fillNeighbors(n2n,cell.nodeIds.data(),4);
        }
        for(const auto & cell : fragment.transferPyramids){
            fillNeighbors(n2n,cell.nodeIds.data(),5);
        }
        for(const auto & cell : fragment.transferPrisms){
            fillNeighbors(n2n,cell.nodeIds.data(),6);
        }
        for(const auto & cell : fragment.transferHexs){
            fillNeighbors(n2n,cell.nodeIds.data(),8);
        }
        Tracer::end("n2n");
        return n2n;
    }
};

}
