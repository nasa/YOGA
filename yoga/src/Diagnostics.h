#pragma once
#include "PartVectorIO.h"
#include <parfait/ExtentBuilder.h>
#include <parfait/ExtentWriter.h>

namespace YOGA{

inline void writeFUN3DPartitionFile(MessagePasser mp, const std::string& filename,const std::vector<long>& owned_gids,bool swap_bytes){
    PartVectorIO exporter(mp,owned_gids,swap_bytes);
    exporter.exportPartVector(filename);
}

template<typename Mesh>
void writeFUN3DPartitionFile(MessagePasser mp, const std::string& filename,const Mesh& mesh,bool swap_bytes){
    std::vector<long> owned_gids;
    for(int i=0;i<mesh.nodeCount();i++)
        if(mp.Rank() == mesh.nodeOwner(i))
            owned_gids.push_back(mesh.globalNodeId(i));
    writeFUN3DPartitionFile(mp,filename,owned_gids,swap_bytes);
}



}