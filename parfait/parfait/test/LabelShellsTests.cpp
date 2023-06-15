#include <RingAssertions.h>
#include <parfait/STL.h>
#include <queue>

TEST_CASE("Label shells") {
    int node_count = 4;
    std::vector<int> triangles = {0, 1, 2, 2, 1, 3};
    std::vector<int> labels;
    int num_labels;
    std::tie(labels, num_labels) = Parfait::STL::labelShells(triangles, node_count);
    REQUIRE(num_labels == 1);
    REQUIRE(labels.at(0) == 0);
    REQUIRE(labels.at(1) == 0);
}

TEST_CASE("Label two disconnected shells") {
    int node_count = 7;
    std::vector<int> triangles = {0, 1, 2, 2, 1, 3, 4, 5, 6};
    std::vector<int> labels;
    int num_labels;
    std::tie(labels, num_labels) = Parfait::STL::labelShells(triangles, node_count);
    REQUIRE(num_labels == 2);
    REQUIRE(labels.at(0) == 0);
    REQUIRE(labels.at(1) == 0);
    REQUIRE(labels.at(2) == 1);
}
