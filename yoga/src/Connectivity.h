#pragma once
#include <set>
#include <vector>
#include "YogaMesh.h"

namespace YOGA {

class Connectivity {
  public:
    static std::vector<std::vector<int>> nodeToCell(const YogaMesh& mesh);
    static std::vector<std::vector<int>> nodeToNode(const YogaMesh& mesh);
};
}