#ifndef DCIF_CHECKER_H
#define DCIF_CHECKER_H
#include <vector>
#include <stdint.h>
#include <limits>
#include <parfait/Point.h>
namespace YOGA {
  namespace DcifChecker {
    void throwIfReceptorsAreInconsistent(const std::vector<int8_t> &iblank,
                                         const std::vector<int64_t> &receptor_ids);

    void throwIfCantRecoverLinearFunction(const std::vector<Parfait::Point<double>>& points,
                                          const std::vector<int64_t> &receptor_ids,
                                          const std::vector<int8_t> &donor_counts,
                                          const std::vector<int64_t> &donor_ids,
                                          const std::vector<double> &donor_weights);

    void throwIfCountsMismatch(const std::vector<int8_t> &iblank,
                               const std::vector<int64_t> &receptor_ids);

    void throwIfBadReceptorIdFound(const std::vector<int8_t> &iblank,
                                   const std::vector<int64_t> &receptor_ids);
  }
}

#endif
