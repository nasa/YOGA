#pragma once

#include <MessagePasser/MessagePasser.h>
#include "LoadBalancer.h"

namespace YOGA {
class VoxelServer {
  public:
    VoxelServer(LoadBalancer& L);
    MessagePasser::Message doWork(MessagePasser::Message& msg);

  private:
    LoadBalancer& loadBalancer;
    Parfait::Extent<double> getNextVoxel();
};
}
