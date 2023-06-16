#pragma once
#include <algorithm>

class GlobalIdTranslator {
  public:
    GlobalIdTranslator(const std::vector<long>& nodes_per_domain)
        : nodes_per_domain(nodes_per_domain), offsets(calcOffsets()) {}

    long globalId(long id_in_domain, int domain) {
        long gid = id_in_domain + offsets[domain];
        return gid;
    }

    static std::vector<int> calcRanksPerComponent(const std::vector<int>& component_id_for_rank) {
        int ncomponents = *std::max_element(component_id_for_rank.begin(), component_id_for_rank.end()) + 1;
        std::vector<int> ranks_per_component(ncomponents);
        for (int n : component_id_for_rank) ranks_per_component[n]++;
        return ranks_per_component;
    }

  private:
    const std::vector<long>& nodes_per_domain;
    const std::vector<long> offsets;

    std::vector<long> calcOffsets() {
        std::vector<long> offset_vec = {0};
        for (auto i : nodes_per_domain) offset_vec.push_back(i + offset_vec.back());
        return offset_vec;
    }
};
