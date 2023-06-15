#pragma once
#include <iostream>
#include <vector>

class DivineLoadBalancer {
  public:
    DivineLoadBalancer(int rank_in, int nproc_in) : rank(rank_in), nproc(nproc_in) {}
    int getAssignedDomain(const std::vector<double>& weights) {
        if (weights.size() == 0) {
            throw std::domain_error("Cannot load balance zero work");
        }
        auto target_loads = calcTargetLoadsFromWeights(weights);
        auto ranks_per_domain = calcRanksPerDomain(target_loads);
        return whatDomainIsRankAssociatedWith(rank, ranks_per_domain);
    }

  private:
    int rank;
    int nproc;

    std::vector<double> calcTargetLoadsFromWeights(std::vector<double> weights) {
        double total_load = 0;
        for (double w : weights) total_load += w;
        for (double& w : weights) w /= total_load;
        return weights;
    }

    int whatDomainIsRankAssociatedWith(int r, const std::vector<int>& ranks_per_domain) {
        int x = r;
        for (int i = 0; i < int(ranks_per_domain.size()); i++) {
            if (x < ranks_per_domain[i])
                return i;
            else
                x -= ranks_per_domain[i];
        }
        throw std::domain_error("Couldn't figure out what domain rank is associated with");
    }

    std::vector<int> calcRanksPerDomain(const std::vector<double>& target_loads) {
        int total_domains = int(target_loads.size());
        std::vector<int> ranks_per_domain(target_loads.size(), 1);
        int remaining_ranks_to_assign = nproc - total_domains;
        for (int i = 0; i < remaining_ranks_to_assign; i++) {
            int worst_domain = getIdOfDomainThatMostNeedsARank(ranks_per_domain, target_loads);
            ranks_per_domain[worst_domain]++;
        }
        return ranks_per_domain;
    }

    int getIdOfDomainThatMostNeedsARank(const std::vector<int>& ranks_per_domain,
                                        const std::vector<double>& target_loads) {
        double current_load = double(ranks_per_domain[0]) / double(nproc);
        double max_delta = target_loads[0] - current_load;
        int max_id = 0;
        for (int i = 1; i < int(ranks_per_domain.size()); i++) {
            current_load = double(ranks_per_domain[i]) / double(nproc);
            double delta = target_loads[i] - current_load;
            if (delta > max_delta) {
                max_delta = delta;
                max_id = i;
            }
        }
        return max_id;
    }
};
