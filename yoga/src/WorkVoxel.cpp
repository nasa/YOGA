#include "WorkVoxel.h"

using namespace std;

namespace YOGA {

void WorkVoxel::addNodes(const vector<TransferNode>& moreNodes,std::vector<int>& new_local_ids) {
    new_local_ids.resize(moreNodes.size());
    for (size_t i=0;i<moreNodes.size();i++) {
        auto& node = moreNodes[i];
        if (global_to_local.count(node.globalId) == 0) {
            int local_id = global_to_local.size();
            global_to_local[node.globalId] = local_id;
            nodes.push_back(node);
            new_local_ids[i] = local_id;
        }
        else{
            new_local_ids[i] = global_to_local[node.globalId];
        }
    }
}

void WorkVoxel::addTets(const std::vector<TransferCell<4>>& more_tets,
    const std::vector<TransferNode>& transfer_nodes,
    const std::vector<int>& new_local_ids) {
    addCells<4>(more_tets,new_local_ids,tets);
}

void WorkVoxel::addPyramids(const std::vector<TransferCell<5>>& morePyramids,
                            const std::vector<TransferNode>& transfer_nodes,
                            const std::vector<int>& new_local_ids) {
    addCells<5>(morePyramids,new_local_ids,pyramids);
}

void WorkVoxel::addPrisms(const std::vector<TransferCell<6>>& morePrisms,
                          const std::vector<TransferNode>& transfer_nodes,
                          const std::vector<int>& new_local_ids) {
    addCells<6>(morePrisms,new_local_ids,prisms);
}

void WorkVoxel::addHexs(const std::vector<TransferCell<8>>& moreHexs,
    const std::vector<TransferNode>& transfer_nodes,
    const std::vector<int>& new_local_ids) {
    addCells<8>(moreHexs,new_local_ids,hexs);
}

}