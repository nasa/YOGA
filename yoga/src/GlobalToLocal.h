#pragma once
#include <map>
#include "YogaMesh.h"

namespace YOGA {

class GlobalToLocal {
  public:
    static std::map<long, int> buildMap(const YogaMesh& mesh) {
        std::map<long, int> g2l;
        for (int i = 0; i < mesh.nodeCount(); ++i) g2l[mesh.globalNodeId(i)] = i;
        return g2l;
    }
};

}
