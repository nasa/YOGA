#pragma once
#include <map>
#include <t-infinity/MeshInterface.h>

namespace inf {
namespace GlobalToLocal {
    std::map<long, int> buildNode(const inf::MeshInterface& mesh);
    std::map<long, int> buildCell(const inf::MeshInterface& mesh);
}

namespace LocalToGlobal {
    std::vector<long> buildCell(const inf::MeshInterface& mesh);
    std::vector<long> buildNode(const inf::MeshInterface& mesh);
}
}
