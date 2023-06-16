#pragma once
#include <vector>
#include <set>
#include <queue>

namespace Parfait {
namespace GraphDistance {
    enum { UNASSIGNED = -1 };
    enum { NO_MAXIMUM = -1 };

    inline std::vector<int> calcGraphDistance(const std::vector<std::vector<int>>& graph,
                                              const std::set<int>& zero_distance_set,
                                              int max_layer = NO_MAXIMUM) {
        std::vector<int> distances(graph.size(), UNASSIGNED);

        std::queue<int> process;
        std::queue<int> next_process;

        for (auto z : zero_distance_set) {
            distances[z] = 0;
            process.push(z);
        }

        if (max_layer == NO_MAXIMUM) {
            max_layer = std::numeric_limits<int>::max();
        }

        for (int layer = 1; layer <= max_layer; layer++) {
            int num_changed = 0;
            while (not process.empty()) {
                int n = process.front();
                process.pop();
                for (const auto& neighbor : graph[n]) {
                    if (distances[neighbor] == UNASSIGNED) {
                        distances[neighbor] = layer;
                        num_changed++;
                        next_process.push(neighbor);
                    }
                }
            }
            if (num_changed == 0) break;
            process = next_process;
        }

        return distances;
    }
}
}
