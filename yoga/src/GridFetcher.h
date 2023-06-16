#pragma once

#include <parfait/Adt3dExtent.h>
#include "MessageTypes.h"
#include "VoxelFragment.h"
namespace YOGA {
class GridFetcher {
  public:
    GridFetcher(const std::vector<VoxelFragment>& fragments);
    MessagePasser::Message doWork(MessagePasser::Message& msg);

  private:
    const std::vector<std::shared_ptr<MessagePasser::Message>> fragment_msgs;
    std::vector<Parfait::Extent<double>> fragment_extents;

    std::vector<std::shared_ptr<MessagePasser::Message>> buildFragmentMessages(const std::vector<VoxelFragment>& frags);
    std::vector<Parfait::Extent<double>> buildFragmentExtents(const std::vector<VoxelFragment>& frags);
    std::vector<int> getOverlappingFragmentIds(const Parfait::Extent<double>& e);
};

}
