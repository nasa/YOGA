#include <RingAssertions.h>
#include <set>
#include <queue>
#include <parfait/GraphDistance.h>

TEST_CASE("Graph distances in serial") {
    std::vector<std::vector<int>> graph = {{0, 1}, {0, 1, 2}, {1, 2, 3}, {2, 3}};
    std::set<int> zero_distance_set = {1};

    SECTION("Calculate the first edge distance") {
        auto distances = Parfait::GraphDistance::calcGraphDistance(graph, zero_distance_set, 1);
        REQUIRE(distances.size() == 4);
        REQUIRE(distances[0] == 1);
        REQUIRE(distances[1] == 0);
        REQUIRE(distances[2] == 1);
        REQUIRE(distances[3] == Parfait::GraphDistance::UNASSIGNED);
    }
}

TEST_CASE("Graph distances in parallel") {
    std::vector<std::vector<int>> graph = {{0, 1}, {0, 1, 2}, {1, 2, 3}, {2, 3}};
    std::set<int> zero_distance_set = {1};

    SECTION("Calculate the first edge distance") {
        auto distances = Parfait::GraphDistance::calcGraphDistance(graph, zero_distance_set, 1);
        REQUIRE(distances.size() == 4);
        REQUIRE(distances[0] == 1);
        REQUIRE(distances[1] == 0);
        REQUIRE(distances[2] == 1);
        REQUIRE(distances[3] == Parfait::GraphDistance::UNASSIGNED);
    }
}
