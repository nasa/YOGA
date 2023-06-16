#pragma once
#include <set>
#include <map>
#include <queue>
#include <Tracer.h>

namespace YOGA {
class FloodFill {
  public:
    FloodFill(const std::vector<std::vector<int>>& node_to_node) : n2n(node_to_node) {}
    void fill(std::vector<int>& node_values, std::set<int> seeds,const std::map<int,int>& allowed_transitions) {
        std::queue<int> queue;
        for (auto seed : seeds) queue.push(seed);
        while (not queue.empty()) {
            int node_id = queue.front();
            queue.pop();
            int current_value = node_values[node_id];
            if (allowed_transitions.count(current_value) == 1)
                node_values[node_id] = allowed_transitions.at(current_value);
            for (int nbr : n2n[node_id]) {
                int nbr_value = node_values[nbr];
                if (allowed_transitions.count(nbr_value) == 1) queue.push(nbr);
            }
        }
    }

    template <typename Sync,typename ParallelMax>
    void fill(
        std::vector<int>& current_values, std::set<int> seeds,const std::map<int,int>& allowed_transitions, Sync sync,ParallelMax parallel_max) {
        Tracer::begin("Serial flood fill");
        fill(current_values, seeds, allowed_transitions);
        Tracer::end("Serial flood fill");
        auto old_values = current_values;
        Tracer::begin("sync");
        sync();
        Tracer::end("sync");
        Tracer::begin("count updates");
        int updates = countUpdates(current_values,old_values,parallel_max);
        Tracer::end("count updates");
        while (updates > 0) {
            seeds = convertUpdatedGhostsToSeeds(current_values,old_values);
            Tracer::begin("Serial flood fill");
            fill(current_values, seeds, allowed_transitions);
            Tracer::end("Serial flood fill");
            Tracer::begin("sync");
            old_values = current_values;
            sync();
            updates = countUpdates(current_values,old_values,parallel_max);
            Tracer::end("sync");
        }
    }

  private:
    const std::vector<std::vector<int>>& n2n;
    int count(const std::vector<int>& values, int target) {
        int n = 0;
        for (int v : values)
            if (v == target) n++;
        return n;
    }

    template<typename ParallelMax>
    int countUpdates(const std::vector<int>& current_values,const std::vector<int>& old_values,ParallelMax parallel_max){
        int n = 0;
        for(int i=0;i<int(current_values.size());i++)
            if(current_values[i] != old_values[i])
                ++n;
        return parallel_max(n);
    }

    std::set<int> convertUpdatedGhostsToSeeds(const std::vector<int>& current_values, const std::vector<int>& old_values){
        std::set<int> seeds;
        for(int i=0;i<int(current_values.size());i++)
            if(current_values[i] != old_values[i])
                seeds.insert(i);
        return seeds;
    }
};
}