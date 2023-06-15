#pragma once
#include <parfait/Extent.h>
namespace YOGA {
class LoadBalancer {
  public:
    virtual int getRemainingVoxelCount() = 0;
    virtual Parfait::Extent<double> getWorkVoxel() = 0;
    virtual ~LoadBalancer() = default;
};
}
