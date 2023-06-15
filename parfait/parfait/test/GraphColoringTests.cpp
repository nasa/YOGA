#include <RingAssertions.h>
#include <parfait/Extent.h>
#include <parfait/GraphColoring.h>

TEST_CASE("Determine how many colors to use") {
    MessagePasser mp(MPI_COMM_WORLD);
    SECTION("A graph with a few edges") {
        std::vector<std::vector<int>> graph{{0, 1}, {1}, {2}, {}};
        auto num_colors = Parfait::GraphColoring::determineNumberOfColorsToBeUsed(mp, graph);
        REQUIRE(num_colors == 4);  // 2*max_stencil_size = 2*2 = 4
    }
    SECTION("A diagonal graph") {
        std::vector<std::vector<int>> graph{{0}, {1}, {2}, {}};
        auto num_colors = Parfait::GraphColoring::determineNumberOfColorsToBeUsed(mp, graph);
        REQUIRE(num_colors == 1);  // special case, is diagonal so we can use a single color
    }
    SECTION("A graph that looks diagonal but isn't") {
        std::vector<std::vector<int>> graph{{4}, {0}, {1}, {}};
        auto num_colors = Parfait::GraphColoring::determineNumberOfColorsToBeUsed(mp, graph);
        REQUIRE(num_colors == 4);  // 2*max_stencil_size
    }
}

TEST_CASE("Can select algorithm") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::vector<std::vector<int>> graph{{0, 1}, {0, 1, 2}, {1, 2, 3}, {2, 3}};
    std::vector<long> gids = {100 * mp.Rank() + 0, 100 * mp.Rank() + 1, 100 * mp.Rank() + 2, 100 * mp.Rank() + 3};
    std::vector<int> owner(4, mp.Rank());
    SECTION("Balanced algorithm") {
        auto colors = Parfait::GraphColoring::color(mp, graph, gids, owner, Parfait::GraphColoring::BALANCED);
        std::set<int> set_colors(colors.begin(), colors.end());
        set_colors = mp.ParallelUnion(set_colors);
        REQUIRE(set_colors.size() == 4);
    }
    SECTION("Greedy algorithm") {
        auto colors = Parfait::GraphColoring::color(mp, graph, gids, owner, Parfait::GraphColoring::GREEDY);
        std::set<int> set_colors(colors.begin(), colors.end());
        set_colors = mp.ParallelUnion(set_colors);
        REQUIRE(set_colors.size() == 3);
    }
}

TEST_CASE("Can color a diagonal graph") {
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<std::vector<int>> graph{{0}, {1}, {2}};
    std::vector<long> gids{mp.Rank() * 10 + 0, mp.Rank() * 10 + 1, mp.Rank() * 10 + 2};
    std::vector<int> owner(3, mp.Rank());

    auto colors = Parfait::GraphColoring::color(mp, graph, gids, owner);
    REQUIRE(colors.size() == 3);
    REQUIRE(colors[0] == 0);
    REQUIRE(colors[1] == 0);
    REQUIRE(colors[2] == 0);
}

TEST_CASE("Select most load balanced color") {
    SECTION("Pick the lowest") {
        std::vector<int> color_counts = {5, 5, 1, 5};
        std::set<int> already_used_colors = {};

        int best_color = Parfait::GraphColoring::selectBestColor(color_counts, already_used_colors);
        REQUIRE(best_color == 2);
    }

    SECTION("Pick the lowest not constrained") {
        std::vector<int> color_counts = {5, 2, 1, 5};
        std::set<int> already_used_colors = {2};

        int best_color = Parfait::GraphColoring::selectBestColor(color_counts, already_used_colors);
        REQUIRE(best_color == 1);
    }

    SECTION("Will pick the first available color") {
        std::vector<int> color_counts = {0, 0, 0, 0};
        std::set<int> already_used_colors = {};

        int best_color = Parfait::GraphColoring::selectBestColor(color_counts, already_used_colors);
        REQUIRE(best_color == 0);
    }
}

TEST_CASE("Label row types") {
    std::vector<std::vector<int>> graph{{0, 1}, {1, 2}, {2, 3}, {}};
    std::vector<int> owners = {0, 0, 0, 1};
    int my_rank = 0;
    auto types = Parfait::GraphColoring::getRowType(graph, owners, my_rank);
    REQUIRE(types.size() == 4);
    REQUIRE(types[0] == Parfait::GraphColoring::INTERIOR);
    REQUIRE(types[1] == Parfait::GraphColoring::INTERIOR);
    REQUIRE(types[2] == Parfait::GraphColoring::FRINGE);
    REQUIRE(types[3] == Parfait::GraphColoring::GHOST);
}

TEST_CASE("Get all current colors in a row") {
    std::vector<int> colors = {0, 1, 2, 3, 4, 5, 6};
    std::vector<int> row = {0, 5, 6};
    std::set<int> row_colors = Parfait::GraphColoring::getCurrentColorsInRow(colors, row);
    REQUIRE(row_colors.size() == 3);
    REQUIRE(row_colors.count(0) == 1);
    REQUIRE(row_colors.count(5) == 1);
    REQUIRE(row_colors.count(6) == 1);
}
