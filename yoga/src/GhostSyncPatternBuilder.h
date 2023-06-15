#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/SyncPattern.h>
#include "MeshSystemInfo.h"

namespace YOGA {
class GhostSyncPatternBuilder {
  public:
    static Parfait::SyncPattern build(const YogaMesh& mesh,
                                      MessagePasser mp);
};
}
