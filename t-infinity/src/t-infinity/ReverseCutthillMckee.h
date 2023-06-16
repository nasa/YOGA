#pragma once
#include <vector>
#include <algorithm>
#include <Tracer.h>
#include <parfait/VectorTools.h>
#include <parfait/Throw.h>
#include <parfait/ToString.h>
#include <random>

namespace inf {
class ReverseCutthillMckee {
  public:
    static std::vector<int> calcNewIds(const std::vector<std::vector<int>>& adjacency);
    static int computeBandwidth(const std::vector<int>& row);
    static std::vector<int> calcNewIdsQ(const std::vector<std::vector<int>>& adjacency,
                                        double p = 1.0);
    static std::vector<int> calcNewIds(const std::vector<std::vector<int>>& adjacency,
                                       const std::vector<bool>& do_own);

    static std::vector<int> putOwnedNodesFirst(const std::vector<int>& old_ids_in_new_ordering,
                                               const std::vector<bool>& do_own);

    static int findHighestValence(const std::vector<std::vector<int>>& adjacency,
                                  const std::set<int>& possibles);

    class Internal {
      public:
        static int findHighestValence(const std::vector<std::vector<int>>& node_to_node) {
            int max_id = 0;
            auto max = node_to_node[0].size();
            for (size_t i = 1; i < node_to_node.size(); i++) {
                auto valence = node_to_node[i].size();
                if (valence > max) {
                    max = valence;
                    max_id = i;
                }
            }
            return max_id;
        }

        static std::vector<int> getSortedAdjacency(const std::vector<int>& neighbors,
                                                   const std::vector<std::vector<int>>& adjacency) {
            std::vector<std::pair<int, int>> neighbors_and_valence;
            for (int nbr : neighbors)
                neighbors_and_valence.emplace_back(nbr, int(adjacency[nbr].size()));
            auto comparator = [](const std::pair<int, int> a, const std::pair<int, int> b) {
                return a.second > b.second;
            };
            std::sort(neighbors_and_valence.begin(), neighbors_and_valence.end(), comparator);
            std::vector<int> sorted_nbrs(neighbors.size());
            for (size_t i = 0; i < neighbors.size(); i++) {
                sorted_nbrs[i] = neighbors_and_valence[i].first;
            }
            return sorted_nbrs;
        }
    };
};
}