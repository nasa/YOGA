#pragma once
#include "t-infinity/Gradients.h"
#include "parfait/MetricTensor.h"

class UnsteadyHessianCombiner {
  public:
    inline UnsteadyHessianCombiner(int node_count, double power = 2.0) : H(node_count) {
        p = power;
        one_over_p = 1.0 / power;
    }
    inline void addHessian(const std::vector<Parfait::Tensor>& A) {
        if (num_added == 0) {
            for (int n = 0; n < int(H.size()); n++) {
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        H[n](i, j) = std::pow(A[n](i, j), p);
                    }
                }
            }
            num_added++;
            return;
        }

        for (int n = 0; n < int(H.size()); n++) {
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    H[n](i, j) += std::pow(A[n](i, j), p);
                }
            }
        }

        num_added++;
    }

    inline std::vector<Parfait::Tensor> getH() {
        for (auto& h : H) {
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    h(i, j) = std::pow(h(i, j) / double(num_added), one_over_p);
                    if (not std::isfinite(h(i, j))) {
                        h.print();
                    }
                    PARFAIT_ASSERT(std::isfinite(h(i, j)), "error with matrix");
                }
            }
        }
        num_added = 0;  // reset
        return H;
    }

  private:
    double p = 2.0;
    double one_over_p = 0.5;
    std::vector<Parfait::Tensor> H;
    int num_added = 0;
};
