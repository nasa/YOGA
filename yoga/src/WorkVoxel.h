#pragma once

#include <map>
#include <mutex>
#include <vector>
#include "TransferNode.h"
#include "VoxelFragment.h"

namespace YOGA {

class WorkVoxel {

  public:
    WorkVoxel() = delete;
    WorkVoxel(const Parfait::Extent<double>& e,std::function<bool(double*,int,double*)> is_in) :
    extent(e), is_in_cell(is_in) {
        int n = 50000;
        nodes.reserve(n);
        tets.reserve(2*n);
    }
    const Parfait::Extent<double> extent;
    std::map<long, int> global_to_local;
    std::function<bool(double*,int,double*)> is_in_cell;

    void addNodes(const std::vector<TransferNode>& moreNodes,std::vector<int>& new_local_ids);
    void addTets(const std::vector<TransferCell<4>>& moreTets, const std::vector<TransferNode>& transfer_nodes,
        const std::vector<int>& new_local_ids);
    void addPyramids(const std::vector<TransferCell<5>>& morePyramids, const std::vector<TransferNode>& transfer_nodes,
        const std::vector<int>& new_local_ids);
    void addPrisms(const std::vector<TransferCell<6>>& morePrisms, const std::vector<TransferNode>& transfer_nodes,
        const std::vector<int>& new_local_ids);
    void addHexs(const std::vector<TransferCell<8>>& moreHexs, const std::vector<TransferNode>& transfer_nodes,
        const std::vector<int>& new_local_ids);

    std::vector<TransferNode> nodes;
    std::vector<TransferCell<4>> tets;
    std::vector<TransferCell<5>> pyramids;
    std::vector<TransferCell<6>> prisms;
    std::vector<TransferCell<8>> hexs;

  private:
    template <int N>
    void convertToWorkCell(std::array<int, N>& cell, const std::vector<int>& new_local_ids) {
        for (int i = 0; i < N; ++i) {
            cell[i] = new_local_ids[cell[i]];
        }
    }
    template <int N>
    void addCells(const std::vector<TransferCell<N>>& more_cells,
                  const std::vector<int>& new_local_ids,
                  std::vector<TransferCell<N>>& cells){
        int i = cells.size();
        cells.insert(cells.end(),more_cells.begin(),more_cells.end());
        for (auto it=cells.begin()+i;it<cells.end();++it){
            convertToWorkCell<N>(it->nodeIds,new_local_ids);
        }
    }
};

}
