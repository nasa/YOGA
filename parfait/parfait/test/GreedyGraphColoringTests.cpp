#include <RingAssertions.h>
#include <parfait/GreedyColoring.h>

TEST_CASE("get colors for a graph of vertices of a square") {
    std::vector<std::vector<int>> graph;
    graph.push_back({1, 3});
    graph.push_back({0, 2});
    graph.push_back({1, 3});
    graph.push_back({0, 2});

    auto colors = GreedyColoring::colorGraph(graph);
    REQUIRE(4 == colors.size());
    for (int color : colors) REQUIRE(color >= 0);

    REQUIRE(0 == colors[0]);
    REQUIRE(1 == colors[1]);
    REQUIRE(0 == colors[2]);
    REQUIRE(1 == colors[3]);
}

TEST_CASE("color graph of two adjacent tets (joined by face 1-2-3)") {
    std::vector<std::vector<int>> graph;
    graph.push_back({1, 2, 3});
    graph.push_back({0, 2, 3, 4});
    graph.push_back({0, 1, 3, 4});
    graph.push_back({0, 1, 2, 4});
    graph.push_back({1, 2, 3});

    auto colors = GreedyColoring::colorGraph(graph);

    SECTION("Ensure that only nodes 0 and 4 share a color") {
        REQUIRE(colors.front() == colors.back());
        int max_color = -1;
        for (int color : colors) max_color = std::max(color, max_color);
        REQUIRE(3 == max_color);
    }
}