#include <RingAssertions.h>
#include <parfait/GraphPlotter.h>

TEST_CASE("Can read and write a graph in serial") {
    std::map<long, std::set<long>> graph;

    graph[0] = {0, 1};
    graph[1] = {1, 2};
    graph[7] = {7, 100};

    auto filename = Parfait::StringTools::randomLetters(5) + ".graph";
    Parfait::Graph::write(filename, graph);
    Parfait::FileTools::waitForFile(filename);

    auto graph_b = Parfait::Graph::read(filename);
    REQUIRE(graph_b.size() == 3);
    REQUIRE(graph_b == graph);
}