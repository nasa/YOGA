#include "PartitionFileMeshLoader.h"
#include "PartVectorIO.h"
#include <t-infinity/Extract.h>
#include <t-infinity/MeshShuffle.h>
#include <t-infinity/Shortcuts.h>
#include "RootPrinter.h"
#include <parfait/FileTools.h>
#include <t-infinity/GhostSyncer.h>

using namespace YOGA;

std::vector<int> expandPartVectorToGhostNodes(MessagePasser mp,std::shared_ptr<MeshInterface> mesh,const std::vector<int>& part){
    std::vector<int> full_part_vector(mesh->nodeCount());
    int current = 0;
    for(int i=0;i<mesh->nodeCount();i++)
        if(mp.Rank() == mesh->nodeOwner(i))
            full_part_vector[i] = part[current++];
    GhostSyncer syncer(mp,mesh);
    syncer.syncNodes(full_part_vector);
    return full_part_vector;
}

std::shared_ptr<MeshInterface> repartitionBasedOnPartFile(MessagePasser mp,
                                                          const std::string& filename,
                                                          std::shared_ptr<MeshInterface> mesh){
    RootPrinter rp(mp.Rank());
    bool swap_bytes = true;
    auto owned_gids = extractOwnedGlobalNodeIds(*mesh);
    if(not Parfait::FileTools::doesFileExist(filename))
        throw std::logic_error("Could not find partition file: "+filename);
    PartVectorIO importer(mp,owned_gids,swap_bytes);

    rp.print("Importing FUN3D part vector from: "+filename);
    auto part = importer.importPartVector(filename);
    long N = mp.ParallelSum(long(part.size()));
    long N2 = mp.ParallelSum(long(owned_gids.size()));
    std::vector<int> nodes_owned_per_rank(mp.NumberOfProcesses(),0);
    for(int r:part)
        nodes_owned_per_rank[r]++;
    mp.ElementalSum(nodes_owned_per_rank,0);
    if(0 == mp.Rank()){
        long N3 = std::accumulate(nodes_owned_per_rank.begin(),nodes_owned_per_rank.end(),0);
        printf("should all be total-nodes:  %li %li %li\n",N,N2,N3);
    }
    part = expandPartVectorToGhostNodes(mp,mesh,part);

    rp.print("Shuffle mesh based on imported part vector");
    return MeshShuffle::shuffleNodes(mp,*mesh,part);
}

std::shared_ptr<MeshInterface> PartFileMeshLoader::load(MPI_Comm comm, std::string filename) const {
    MessagePasser mp(comm);
    auto mesh = inf::shortcut::loadMesh(mp,filename);
    return repartitionBasedOnPartFile(mp,partition_filename,mesh);
}

std::shared_ptr<inf::MeshLoader> createPreProcessor() { return std::make_shared<PartFileMeshLoader>(); }