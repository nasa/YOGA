#pragma once
#include <vector>
#include "MeshSystemInfo.h"
#include "YogaMesh.h"

namespace YOGA {
class OverlapMask {
  public:
    static std::vector<bool> buildNodeMask(const YogaMesh& m, MeshSystemInfo& info);

  private:
    static bool isPointInOverlap(const Parfait::Point<double>& p, MeshSystemInfo& info);
};
}
