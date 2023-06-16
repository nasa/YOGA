#pragma once
#include <t-infinity/Shortcuts.h>
#include <t-infinity/VectorFieldAdapter.h>

namespace YOGA{

template<typename Mesh>
void visualizePartitionExtents(MessagePasser mp, const std::string& filename, const Mesh& mesh){
    auto extent = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    for(int i=0;i<mesh.nodeCount();i++) {
        Parfait::Point<double> p;
        mesh.nodeCoordinate(i,p.data());
        Parfait::ExtentBuilder::add(extent, p);
    }
    auto all_extents = mp.Gather(extent,0);
    std::vector<int> partition_ids(mp.NumberOfProcesses());
    for(int i=0;i<mp.NumberOfProcesses();i++)
        partition_ids[i] = i;
    if(0 == mp.Rank())
        Parfait::ExtentWriter::write(filename,all_extents,partition_ids);
}

void visualizePartitions(MessagePasser mp, const std::string& filename, std::shared_ptr<inf::MeshInterface> mesh){
    std::vector<int> node_owner(mesh->nodeCount());
    for(int i=0;i<mesh->nodeCount();i++)
        node_owner[i] = mesh->nodeOwner(i);
    auto field = std::make_shared<inf::VectorFieldAdapter>("node-owner","node",node_owner);
    inf::shortcut::visualize(filename,mp,mesh,{field});
}
}