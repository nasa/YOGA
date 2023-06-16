#include <RingAssertions.h>
#include <t-infinity/MeshMechanics.h>
#include "AdaptationTestFixtures.h"

using namespace inf;

int getIdOfCriticalNode(Parfait::Point<double> p,const std::vector<int>& inserted_node_ids,experimental::MeshBuilder& builder){
    for(int id:inserted_node_ids){
        Parfait::Point<double> b = builder.mesh->node(id);
        if(p.approxEqual(b))
            return id;
    }
    return -1;
}

bool alreadyInserted(Parfait::Point<double> p,const std::vector<int>& inserted_node_ids,experimental::MeshBuilder& builder){
    // TODO: currently not handling if we found a matching point, but that point isn't on geometry.  Should probably make that
    // point be on the geometry retroactively somehow.
    int id = getIdOfCriticalNode(p,inserted_node_ids,builder);
    return id > -1;
}

TEST_CASE("try to make some quads"){
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto thing = getInitialUnitQuad(mp);
    auto& builder = thing.builder;

    auto calc_edge_length = buildIsotropicCallback(0.01);
    std::string viz_file = "make-quads";
    auto mechanics = Mechanics(mp,viz_file);

    mechanics.copyBuilderAndSyncAndVisualize(builder,calc_edge_length);

    mechanics.insertNode(builder,{.25,.25,0});
    mechanics.copyBuilderAndSyncAndVisualize(builder,calc_edge_length);


    //mechanics.insertNode(builder,{.75,.75,0});
    //mechanics.copyBuilderAndSyncAndVisualize(builder,calc_edge_length);

    std::array<int,2> edge {3,4};
    mechanics.combineNeighborsToQuad(builder, edge);
    mechanics.copyBuilderAndSyncAndVisualize(builder,calc_edge_length);

}
