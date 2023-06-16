#pragma once
#include <parfait/Point.h>
#include <vector>
#include "YogaMesh.h"

namespace YOGA {

class DistanceFieldAdapter {
  public:
    static std::vector<double> getNodeDistances(const YogaMesh& m,
                                                const std::vector<std::vector<Parfait::Point<double>>>& surfaces);

  private:
    static std::vector<Parfait::Point<double>> extractQueryPoints(const YogaMesh& mesh);
    static std::vector<int> extractGridIds(const YogaMesh& mesh);
    static void modifyDistanceOnInterpolationBoundaries(const YogaMesh& m,std::vector<std::vector<double>>& distance);
};

}
