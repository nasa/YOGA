#pragma once
#include <vector>
#include "Adt3dPoint.h"
#include "Point.h"
#include "Throw.h"

namespace Parfait {
class UniquePoints {
  public:
    inline UniquePoints(const Parfait::Extent<double>& domain) : adt(domain) {}

    inline int getNodeId(const Parfait::Point<double>& p, double tol = 1.0e-8) {
        int node_id;
        if (adt.isPointInAdt(p, node_id, tol)) {
            return node_id;
        } else {
            adt.store(next_node_id, p);
            points.push_back(p);
            return next_node_id++;
        }
    }

    inline int getNodeIdDontInsert(const Parfait::Point<double>& p, double tol = 1.0e-8) const {
        int node_id;
        if (adt.isPointInAdt(p, node_id, tol)) {
            return node_id;
        }
        PARFAIT_THROW("Could not find node in UniquePoints set " + p.to_string("%e"));
    }

    std::vector<Parfait::Point<double>> points;

  private:
    int next_node_id = 0;
    Parfait::Adt3DPoint adt;
};
}
