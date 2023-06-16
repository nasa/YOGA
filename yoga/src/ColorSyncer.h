#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/SyncPattern.h>
#include <parfait/SyncField.h>

namespace YOGA {
class ColorSyncer {
  public:
    ColorSyncer(MessagePasser mp, const std::map<long, int>& g2l, const Parfait::SyncPattern& sync_pattern)
        : mp(mp), g2l(g2l), sync_pattern(sync_pattern) {}
    void sync(std::vector<int>& colors) const { Parfait::syncVector(mp, colors, g2l, sync_pattern); }

  private:
    MessagePasser mp;
    const std::map<long, int>& g2l;
    const Parfait::SyncPattern& sync_pattern;
};
}
