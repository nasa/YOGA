#include "DcifChecker.h"
#include <parfait/Point.h>
#include "LinearTestFunction.h"

namespace YOGA {
  void DcifChecker::throwIfCantRecoverLinearFunction(const std::vector<Parfait::Point<double>>& points,
                                                     const std::vector<int64_t> &receptor_ids,
                                                     const std::vector<int8_t> &donor_counts,
                                                     const std::vector<int64_t> &donor_ids,
                                                     const std::vector<double> &donor_weights) {
      if(receptor_ids.size() != donor_counts.size())
          throw std::logic_error("Size mismatch");
      if(donor_ids.size() != donor_weights.size())
          throw std::logic_error("Size mismatch");

      printf("DcifChecker: testing recovery of linear test function\n");
      double tolerance = 1.0e-4;
      int offset = 0;
      double error_rms = 0;
      double error_min = std::numeric_limits<double>::max();
      double error_max = 0;
      for (size_t i = 0; i < receptor_ids.size(); i++) {
          auto id = receptor_ids[i];
          auto p = points[id];
          double actual = linearTestFunction(p);

          int n = donor_counts[i];

          double interpolated = 0.0;
          double weight_sum = 0.0;
          for (int j = 0; j < n; j++) {
              auto donor = points[donor_ids[offset+j]];
              double weight = donor_weights[offset+j];
              weight_sum += weight;
              interpolated += weight * linearTestFunction(donor);
          }
          if(std::abs(1.0-weight_sum) > 1e-6){
              throw std::logic_error("Expected weights to sum to 1.0, got "+std::to_string(weight_sum));
          }

          double delta = fabs(interpolated - actual);
          error_rms += delta * delta;
          error_min = delta < error_min ? delta : error_min;
          error_max = delta > error_max ? delta : error_max;
          if (delta > tolerance) {
              printf("Error:  rms %e min %e max %e\n",error_rms,error_min,error_max);

              throw std::logic_error("Doesn't recover linear function.");
          }

          offset += n;
      }
      error_rms /= receptor_ids.size();
      error_rms = sqrt(error_rms);
      printf("Interpolation error:\n");
      printf("----  rms: %e max: %e min: %e (%li interpolations)\n",
             error_rms, error_max, error_min, receptor_ids.size());
  }

  void DcifChecker::throwIfReceptorsAreInconsistent(const std::vector<int8_t> &iblank,
                                                           const std::vector<int64_t> &receptor_ids) {
      printf("-- Checking consistency between iblank and receptor ids\n");
      throwIfBadReceptorIdFound(iblank, receptor_ids);
      throwIfCountsMismatch(iblank, receptor_ids);
      printf("-- Consistency check complete.\n");
  }

  void DcifChecker::throwIfCountsMismatch(const std::vector<int8_t> &iblank,
                                                 const std::vector<int64_t> &receptor_ids) {
      size_t    n = 0;
      for (int b:iblank)
          if (-1 == b)
              n++;
      if (receptor_ids.size() != n) {
          std::string msg = "Number of nodes blanked (" + std::to_string(n);
          msg += ") doesn't match number of receptors (";
          msg += std::to_string(receptor_ids.size());
          msg += ")";
          throw std::logic_error( msg);
      } else
          printf("-- Receptor counts match (%li)\n", n);
  }

  void DcifChecker::throwIfBadReceptorIdFound(const std::vector<int8_t> &iblank,
                                                     const std::vector<int64_t> &receptor_ids) {
      printf("---- Checking for bad receptor ids\n");
      for (int id:receptor_ids) {
          if (int(iblank.size()) <= id) throw std::range_error("Receptor id too large");
          else if (0 > id) throw std::range_error("Negative receptor id");
          else if (-1 != iblank[id])
              throw std::logic_error("Receptor not marked in iblank: "+std::to_string(id));
      }
      printf("---- Done.\n");
  }
}