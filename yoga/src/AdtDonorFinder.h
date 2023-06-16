#pragma once
#include <parfait/Adt3dExtent.h>
#include "Receptor.h"
#include "WorkVoxel.h"

namespace YOGA {

class AdtDonorFinder {
  public:
    AdtDonorFinder(WorkVoxel& vox);
    void fillAdt(Parfait::Adt3DExtent& adt, int component);
    std::vector<int> getComponentIdsForVoxel(const WorkVoxel& w);
    int getCellSize(int cell_id) const;
    int getIdInType(int cell_id) const;
    std::vector<CandidateDonor> findDonors(Parfait::Point<double>& p, int associatedComponent);
    int getCellComponent(int cellId) const;

  private:
    WorkVoxel& workVoxel;
    Parfait::Extent<double> voxel_extent;
    std::vector<int> component_ids;
    std::vector<std::shared_ptr<Parfait::Adt3DExtent>> adts;

    Parfait::Extent<double> getExtentForVoxel(WorkVoxel& workVoxel);
};

}
