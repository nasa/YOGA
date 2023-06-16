#pragma once

namespace YOGA {
struct DonorCloud {
    DonorCloud() = default;
    DonorCloud(int n) : node_ids(n), weights(n) {}
    std::vector<int> node_ids;
    std::vector<double> weights;
};
}