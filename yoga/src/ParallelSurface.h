#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/Point.h>
#include <vector>
#include "YogaMesh.h"

namespace YOGA {

class ParallelSurface {
  public:
    static std::vector<Parfait::Point<double>> getLocalSurfacePointsInComponent(const YogaMesh& m, int component);
    static std::vector<std::vector<Parfait::Point<double>>> buildSurfaces(MessagePasser mp, const YogaMesh& m);
    static int countComponents(MessagePasser mp, const YogaMesh& m);

  private:
    static std::vector<bool> generateNodeMask(const YogaMesh& m, int component);
};
}
