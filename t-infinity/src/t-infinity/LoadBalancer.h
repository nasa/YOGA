#pragma once
#include "DivineLoadBalancer.h"
#include <vector>

namespace inf {
class LoadBalancer {
  public:
    LoadBalancer(int rank, int nproc) : loadBalancer(rank, nproc) {}

    int assignedDomain(const std::vector<double>& weights) {
        return loadBalancer.getAssignedDomain(weights);
    }

  private:
    DivineLoadBalancer loadBalancer;
};
}
