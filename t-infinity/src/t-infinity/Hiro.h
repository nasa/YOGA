#pragma once
#include <vector>
#include <parfait/Point.h>

namespace inf {

class Cell;

class Hiro {
  public:
    static Parfait::Point<double> calcCentroid(const Cell& cell, double p);

    static Parfait::Point<double> calcCentroidFor2dElement(
        const std::vector<Parfait::Point<double>>& points, double p);
};

}
