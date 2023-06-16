#include <set>
#include <RingAssertions.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/ReverseCutthillMckee.h>
#include <parfait/ToString.h>

using namespace inf;

void printBandwith(const std::vector<std::vector<int>>& n2n) {
    int n = n2n.size();
    for (auto& nbrs : n2n) {
        for (int i = 0; i < n; i++) {
            if (std::find(nbrs.begin(), nbrs.end(), i) != nbrs.end())
                printf("%d ", i);
            else
                printf("  ");
        }
        printf("\n");
    }
}

TEST_CASE("Find highest adjacency from a set of possibles") {
    std::vector<std::vector<int>> node_to_node{
        {1, 2, 6}, {0, 2, 3, 5, 6}, {0, 1, 4, 6}, {1, 2, 4, 5, 6}, {2, 3, 6}, {1, 3, 6}, {0, 1, 2, 3, 4, 5}};

    REQUIRE(ReverseCutthillMckee::findHighestValence(node_to_node, {}) == -1);
    REQUIRE(ReverseCutthillMckee::findHighestValence(node_to_node, {0}) == 0);
    REQUIRE(ReverseCutthillMckee::findHighestValence(node_to_node, {1}) == 1);
    REQUIRE(ReverseCutthillMckee::findHighestValence(node_to_node, {1, 2}) == 1);
}

TEST_CASE("Work with two disjoint graphs") {
    std::vector<std::vector<int>> node_to_node{{0, 1}, {0, 1, 2}, {1, 2}, {3}, {4, 5, 6}, {5, 4}, {5, 6}};

    auto new_ids = ReverseCutthillMckee::calcNewIds(node_to_node);
    REQUIRE(node_to_node.size() == new_ids.size());
}

TEST_CASE("Shuffle a region of a vector"){
    std::vector<int> vec(10);
    std::iota(vec.begin(), vec.end(), 0);
    printf("Vector %s\n", Parfait::to_string(vec).c_str());

    std::mt19937 gen;
    gen.seed(42);

    std::shuffle(vec.begin()+5, vec.begin()+8, gen);
    printf("Vector %s\n", Parfait::to_string(vec).c_str());
}

TEST_CASE("Create new node ids based on adjacency") {
    using namespace inf;
    std::vector<std::vector<int>> node_to_node{
        {1, 2, 6}, {0, 2, 3, 5, 6}, {0, 1, 4, 6}, {1, 2, 4, 5, 6}, {2, 3, 6}, {1, 3, 6}, {0, 1, 2, 3, 4, 5}};

    int max_degree = ReverseCutthillMckee::Internal::findHighestValence(node_to_node);
    REQUIRE(6 == max_degree);

    auto old_to_new = ReverseCutthillMckee::calcNewIds(node_to_node);
    REQUIRE(node_to_node.size() == old_to_new.size());

    REQUIRE(old_to_new[6] == 0);
    REQUIRE(old_to_new[1] == 1);
    REQUIRE(old_to_new[3] == 2);
    REQUIRE(old_to_new[2] == 3);
    REQUIRE(old_to_new[0] == 4);
    REQUIRE(old_to_new[4] == 5);
    REQUIRE(old_to_new[5] == 6);
}

TEST_CASE("Do Anderson Q reordering"){
    std::vector<std::vector<int>> node_to_node{
        {1, 2, 6}, {0, 2, 3, 5, 6}, {0, 1, 4, 6}, {1, 2, 4, 5, 6}, {2, 3, 6}, {1, 3, 6}, {0, 1, 2, 3, 4, 5}};
    auto old_to_new = ReverseCutthillMckee::calcNewIdsQ(node_to_node);
    REQUIRE(old_to_new.size() == node_to_node.size());
}
