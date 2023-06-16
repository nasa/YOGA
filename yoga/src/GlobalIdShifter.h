#pragma once
#include <MessagePasser/MessagePasser.h>
#include <vector>

namespace YOGA{

class GlobalIdShifter{
  public:

    static long calcGlobalIdOffsetForComponentGrid(MessagePasser mp, const std::vector<long>& ids, int component_id){
        auto nodes_per_grid = countNodesPerComponentGrid(mp,ids,component_id);
        auto offsets = convertNodeCountsToOffsets(nodes_per_grid);
        return offsets[component_id];
    }

    static std::vector<long> countNodesPerComponentGrid(MessagePasser mp, const std::vector<long>& gids, int component_id){
        int ngrids = mp.ParallelMax(component_id) + 1;
        std::vector<long> nodes_per_grid(ngrids,0);
        long my_biggest_gid = 0;
        for(long gid:gids)
            my_biggest_gid = std::max(gid, my_biggest_gid);
        nodes_per_grid[component_id] = my_biggest_gid;
        nodes_per_grid = mp.ElementalMax(nodes_per_grid);

        for(long& n:nodes_per_grid)
            n++;
        return nodes_per_grid;
    }

    static std::vector<long> convertNodeCountsToOffsets(const std::vector<long>& nodes_per_grid){
        int ngrids = nodes_per_grid.size();
        std::vector<long> offsets(ngrids,0);
        for(int i=1;i<ngrids;i++)
            offsets[i] = offsets[i-1] + nodes_per_grid[i-1];
        return offsets;
    }

};

}
