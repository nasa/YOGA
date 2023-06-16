#pragma once
#include <queue>
#include <parfait/SyncPattern.h>
#include <parfait/SyncField.h>
#include <parfait/MapTools.h>
#include <parfait/Throw.h>
#include <parfait/Throw.h>
#include <parfait/GraphPlotter.h>
#include <parfait/ToString.h>
#include <parfait/SyncPattern.h>
#include "Throw.h"

namespace Parfait {

class GraphColoring {
  public:
    enum { INTERIOR, FRINGE, GHOST };
    enum Algorithm { BALANCED, GREEDY };

    static bool areAllOwned(const std::vector<int>& neighbors, const std::vector<int>& owners, int my_rank) {
        for (auto& c : neighbors) {
            if (owners[c] != my_rank) return false;
        }
        return true;
    }

    static std::vector<int> getRowType(const std::vector<std::vector<int>>& graph,
                                       const std::vector<int>& owner,
                                       int my_rank) {
        std::vector<int> row_type(graph.size());
        for (int row = 0; row < int(graph.size()); row++) {
            if (owner[row] != my_rank)
                row_type[row] = GHOST;
            else if (areAllOwned(graph[row], owner, my_rank))
                row_type[row] = INTERIOR;
            else
                row_type[row] = FRINGE;
        }
        return row_type;
    }

    static void color(
        const std::vector<std::vector<int>>& graph,
        std::vector<int>& colors,
        std::vector<int>& color_counts,
        Algorithm algorithm = BALANCED,
        std::function<bool(int)> should_assign = [](int) { return true; }) {
        typedef std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<>>
            PriorityQueue;

        PARFAIT_ASSERT(graph.size() == colors.size(), "graph, colors size mismatch");

        //--- queue up all rows, with priority given to high valence rows
        PriorityQueue queue;
        for (int row = 0; row < int(graph.size()); row++) {
            if (should_assign(row)) {
                queue.push(std::make_pair(int(graph[row].size()), row));
            }
        }

        if (queue.empty()) return;

        while (not queue.empty()) {
            int row = queue.top().second;
            queue.pop();

            auto used_colors = getCurrentColorsInRow(colors, graph[row]);
            if (used_colors.size() == color_counts.size()) {
                printf("All colors used in row %d, colors [%s], columns: [%s]\n",
                       row,
                       Parfait::to_string(used_colors).c_str(),
                       Parfait::to_string(graph[row]).c_str());
            }
            int row_color;
            if (algorithm == BALANCED) {
                row_color = selectBestColor(color_counts, used_colors);
            } else if (algorithm == GREEDY) {
                row_color = selectLowestColor(color_counts, used_colors);
            } else {
                PARFAIT_THROW("Invalid color selection algorithm");
            }
            color_counts[row_color]++;
            colors[row] = row_color;
        }
    }

    static int largestStencilSize(const std::vector<std::vector<int>>& graph) {
        int biggest = 0;
        for (int row = 0; row < int(graph.size()); row++) {
            int stencil_size = int(graph[row].size());
            if (stencil_size == 1) {
                // This graph is missing diagonals.
                // The stencil size is still 2.
                if (graph[row].front() != row) stencil_size = 2;
            }
            biggest = std::max(stencil_size, biggest);
        }
        return biggest;
    }

    static int determineNumberOfColorsToBeUsed(MessagePasser mp, const std::vector<std::vector<int>>& graph) {
        int max_expected_colors = mp.ParallelMax(largestStencilSize(graph));
        if (max_expected_colors == 1) return 1;
        return 2 * max_expected_colors;
    }

    static std::vector<int> color(MessagePasser mp,
                                  const std::vector<std::vector<int>>& graph,
                                  const std::vector<long>& gids,
                                  const std::vector<int>& owner,
                                  Algorithm algorithm = BALANCED) {
        PARFAIT_ASSERT(graph.size() == gids.size(), "graph, gids size mimatch");
        PARFAIT_ASSERT(graph.size() == owner.size(), "graph, owner size mimatch");
        std::vector<int> colors(graph.size(), UNASSIGNED);

        auto row_types = getRowType(graph, owner, mp.Rank());
        auto is_interior = [&](int row) { return row_types[row] == INTERIOR; };
        auto is_fringe = [&](int row) { return row_types[row] == FRINGE; };

        int num_colors_to_use = determineNumberOfColorsToBeUsed(mp, graph);
        std::vector<int> color_counts(num_colors_to_use, 0);
        color(graph, colors, color_counts, algorithm, is_interior);

        auto g2l = Parfait::MapTools::invert(gids);
        auto sync_pattern = Parfait::SyncPattern(mp, gids, owner);

        for (int rank = 0; rank < mp.NumberOfProcesses(); rank++) {
            if (mp.Rank() == rank) {
                color(graph, colors, color_counts, algorithm, is_fringe);
            }
            Parfait::syncVector(mp, colors, g2l, sync_pattern);
        }

        return colors;
    }

    static std::set<int> getCurrentColorsInRow(const std::vector<int>& colors, const std::vector<int>& row_neighbors) {
        std::set<int> row_colors;
        for (auto c : row_neighbors) {
            auto color = colors[c];
            if (color != UNASSIGNED) row_colors.insert(colors[c]);
        }
        return row_colors;
    }

    static int selectBestColor(const std::vector<int>& color_counts, const std::set<int>& already_used_colors) {
        int best_color = -2;
        int min_count = std::numeric_limits<int>::max();

        if (already_used_colors.size() == color_counts.size()) {
            PARFAIT_WARNING("Have to create a new color");
        }

        for (int color = 0; color < int(color_counts.size()); color++) {
            if (already_used_colors.count(color) == 0) {
                if (color_counts[color] < min_count) {
                    best_color = color;
                    min_count = color_counts[color];
                }
            }
        }
        return best_color;
    }

    static int selectLowestColor(const std::vector<int>& color_counts, const std::set<int>& already_used_colors) {
        for (int color = 0; color < int(color_counts.size()); color++) {
            if (already_used_colors.count(color) == 0) {
                return color;
            }
        }
        PARFAIT_THROW("Ran out of colors!");
    }

    static bool isEdgeBidirectional(const std::vector<std::vector<int>>& graph, int a, int b) {
        bool a_points_to_b = false;
        bool b_points_to_a = false;
        for (int column : graph[a]) {
            if (column == b) a_points_to_b = true;
        }
        for (int column : graph[b]) {
            if (column == a) b_points_to_a = true;
        }

        return a_points_to_b and b_points_to_a;
    }

    static bool checkColoring(MessagePasser mp,
                              const std::vector<std::vector<int>>& graph,
                              const std::vector<int>& owners,
                              const std::vector<int>& color) {
        bool good = true;
        for (int row = 0; row < int(graph.size()); row++) {
            if (owners[row] != mp.Rank()) continue;
            int row_color = color[row];
            for (auto col : graph[row]) {
                int col_color = color[col];
                if (row == col) continue;
                if (row_color == col_color) {
                    printf("row %d, color %d, connected to column %d, color %d\n", int(row), row_color, col, col_color);
                    printf("Edge is %sbidirectional\n", (isEdgeBidirectional(graph, row, col)) ? ("") : ("NOT"));
                    good = false;
                }
            }
        }
        return mp.ParallelAnd(good);
    }

    static void printStatistics(MessagePasser mp,
                                const std::vector<std::vector<int>>& graph,
                                const std::vector<int>& owners,
                                const std::vector<int>& colors) {
        std::set<int> unique_colors(colors.begin(), colors.end());
        unique_colors = mp.ParallelUnion(unique_colors);
        int num_colors = unique_colors.size();
        mp_rootprint("Number of unique colors: %d\n", num_colors);

        auto largest_stencil = largestStencilSize(graph);
        largest_stencil = mp.ParallelMax(largest_stencil);
        mp_rootprint("Largest stencil, n = %d\n", largest_stencil);

        std::vector<int> color_counts(num_colors);
        for (int row = 0; row < int(owners.size()); row++) {
            if (owners[row] == mp.Rank()) color_counts[colors[row]]++;
        }

        std::vector<double> min(num_colors), max(num_colors), total(num_colors);
        for (int c = 0; c < num_colors; c++) {
            min[c] = mp.ParallelMin(double(color_counts[c]));
            max[c] = mp.ParallelMax(double(color_counts[c]));
            total[c] = mp.ParallelSum(double(color_counts[c]));
            auto average = total[c] / double(mp.NumberOfProcesses());
            double load_percent = 100 * average / max[c];
            mp_rootprint(
                "Color %d, min %e, max %e, total %e, balance %2.0f\n", c, min[c], max[c], total[c], load_percent);
        }
    }

    inline static std::map<int, Parfait::SyncPattern> makeSyncPatternsForEachColor(MessagePasser mp,
                                                                                   const std::vector<int>& colors,
                                                                                   const std::vector<long>& gids,
                                                                                   const std::vector<int>& owners) {
        std::set<int> unique_colors(colors.begin(), colors.end());
        unique_colors = mp.ParallelUnion(unique_colors);
        std::map<int, Parfait::SyncPattern> color_sync_patterns;

        for (auto c : unique_colors) {
            std::vector<long> have, need;
            std::vector<int> need_from;
            for (unsigned long i = 0; i < colors.size(); i++) {
                if (colors[i] == c) {
                    if (mp.Rank() == owners[i]) {
                        have.push_back(gids[i]);
                    } else {
                        need.push_back(gids[i]);
                        need_from.push_back(owners[i]);
                    }
                }
            }
            color_sync_patterns.emplace(c, Parfait::SyncPattern{mp, have, need, need_from});
        }
        return color_sync_patterns;
    }

  private:
    enum { UNASSIGNED = -1 };
};
}