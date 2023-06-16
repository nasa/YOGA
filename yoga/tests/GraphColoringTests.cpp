#include <RingAssertions.h>
#include <GraphColoring.h>
#include <ParallelColorCombinator.h>
#include <set>
#include <MessagePasser/MessagePasser.h>
#include <parfait/SyncPattern.h>
#include <t-infinity/CartMesh.h>
#include "YogaMesh.h"

int countUnique(const std::vector<int>& colors){
    return std::set<int>(colors.begin(),colors.end()).size();
}

int findMax(const std::vector<int>& colors){
    int m = 0;
    for(auto x:colors)
        m = std::max(m,x);
    return m;
}

using namespace YOGA;

TEST_CASE("given a graph, generate unique ids for disjoint subgraphs"){
    typedef std::vector<std::vector<int>> Graph;

    SECTION("an empty graph yields an empty color set"){
        Graph graph;
        auto colors = GraphColoring::colorDisjointSubgraphs(graph);
        REQUIRE(colors.empty());
    }

    SECTION("a single node yield a single color") {
        Graph graph{{0}};
        auto colors = GraphColoring::colorDisjointSubgraphs(graph);
        REQUIRE(1 == colors.size());
        REQUIRE(0 == colors[0]);
    }

    SECTION("two connected nodes yield a single color"){
        Graph graph{{1},{0}};
        auto colors = GraphColoring::colorDisjointSubgraphs(graph);
        REQUIRE(2 == colors.size());
        REQUIRE(0 == colors[0]);
        REQUIRE(0 == colors[1]);
    }

    SECTION("two disjoint nodes yield two colors"){
        Graph graph{{0},{1}};
        auto colors = GraphColoring::colorDisjointSubgraphs(graph);

        REQUIRE(2 == countUnique(colors));
        REQUIRE(1 == findMax(colors));
    }

    SECTION("chain of connected nodes form a single graph"){
        Graph graph {{1},{0,2},{1,3},{2,4},{3}};
        auto colors = GraphColoring::colorDisjointSubgraphs(graph);
        REQUIRE(1 == countUnique(colors));
    }

    SECTION("two chains form two subgraphs"){
        Graph graph {{1},{0},{3},{2,4},{3}};
        auto colors = GraphColoring::colorDisjointSubgraphs(graph);
        REQUIRE(2 == countUnique(colors));
        REQUIRE(1 == findMax(colors));
    }
}

class MockSyncer {
  public:
    void sync(std::vector<int>& colors) const {
        colors[0] = 7;
    }
};


TEST_CASE("sync parallel graph colors"){
    MessagePasser mp(MPI_COMM_WORLD);
    if(mp.NumberOfProcesses() != 2)
        return;
    typedef std::vector<std::vector<int>> Graph;

    SECTION("given a contiguous graph, for which each processor has a unique color id, reduce to a single color in parallel") {
        Graph graph {{1},{0}};
        MockSyncer syncer;
        std::vector<int> unique_colors{9,9};
        std::vector<int> owners {1, 0};

        auto reduced_colors = ParallelColorCombinator::combineToOrdinals(graph, unique_colors, owners, mp, syncer);
        REQUIRE(2 == reduced_colors.size());
        REQUIRE(1 == countUnique(reduced_colors));
        REQUIRE(0 == reduced_colors[0]);
    }
}
