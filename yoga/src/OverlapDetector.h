#pragma once
#include <parfait/Adt3dExtent.h>

namespace YOGA {

class OverlapDetector {
  public:
    OverlapDetector(const std::vector<Parfait::Extent<double>>& extents)
        : adt(Parfait::ExtentBuilder::getBoundingBox(extents)), extents(extents) {
        for (size_t i = 0; i < extents.size(); i++) adt.store(i, extents[i]);
    }

    bool doesOverlap(const Parfait::Extent<double>& e, int rank) const {
        return e.intersects(extents[rank]);
    }

    std::vector<int> getOverlappingRanks(const Parfait::Extent<double>& e) const{
        return adt.retrieve(e);
    }

    void getOverlappingRanks(const Parfait::Point<double>& p, std::vector<int>& ranks) const {
        adt.retrieve({p, p}, ranks);
    }

  private:
    Parfait::Adt3DExtent adt;
    const std::vector<Parfait::Extent<double>>& extents;
};

}