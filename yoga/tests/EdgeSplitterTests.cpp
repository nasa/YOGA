#include <RingAssertions.h>
#include <array>
#include <parfait/CGNSElements.h>

typedef std::array<int,4> Tet;


std::vector<Tet> splitEdge(Tet tet, int edge, int new_node_id){
    auto left = tet;
    auto right = tet;
    auto edges = Parfait::CGNS::Tet::edge_to_node;
    left[edges[edge][1]] = new_node_id;
    right[edges[edge][0]] = new_node_id;
    return {left,right};
}

TEST_CASE("split an edge topologically"){
    Tet tet {0,1,2,3};
    auto tets = splitEdge(tet, 0, 4);
    REQUIRE(2 == tets.size());

    Tet left {0,4,2,3};
    Tet right {4,1,2,3};

    REQUIRE(left == tets[0]);
    REQUIRE(right == tets[1]);
}
