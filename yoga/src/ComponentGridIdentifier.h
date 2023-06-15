#pragma once
#include "GraphColoring.h"
#include "YogaMesh.h"
#include "Connectivity.h"
#include <MessagePasser/MessagePasser.h>
#include <Tracer.h>
#include "GhostSyncPatternBuilder.h"
#include "ParallelColorCombinator.h"
#include "ColorSyncer.h"

namespace YOGA{

class ComponentGridIdentifier {
  public:
    static std::vector<int> determineGridIdsForNodes(MessagePasser mp,const YogaMesh& mesh){
        auto n2n = YOGA::Connectivity::nodeToNode(mesh);
        auto colors = YOGA::GraphColoring::colorDisjointSubgraphs(n2n);

        int my_color_offset = calcColorOffset(mp,colors);
        for(auto& c:colors)
            c += my_color_offset;

        std::vector<int> owning_rank(mesh.nodeCount(),0);
        for(int i=0;i<mesh.nodeCount();i++)
            owning_rank[i] = mesh.nodeOwner(i);

        Tracer::begin("build sync pattern");
        auto sync_pattern = YOGA::GhostSyncPatternBuilder::build(   mesh,mp);
        Tracer::end("build sync pattern");

        std::map<long,int> g2l;
        for(int i=0;i<mesh.nodeCount();i++)
            g2l[mesh.globalNodeId(i)] = i;

        YOGA::ColorSyncer syncer(mp,g2l,sync_pattern);
        auto ordinal_colors = YOGA::ParallelColorCombinator::combineToOrdinals(n2n,colors,owning_rank,mp,syncer);

        return ordinal_colors;
    }
  private:

    static int calcColorOffset(MessagePasser& mp, const YOGA::GraphColoring::Colors& colors) {
        auto colors_per_rank = mp.Gather(int(colors.size()));
        int my_color_offset = 0;
        for (int i = 0; i < mp.Rank(); i++) my_color_offset += colors_per_rank[i];
        return my_color_offset;
    }
};
}
