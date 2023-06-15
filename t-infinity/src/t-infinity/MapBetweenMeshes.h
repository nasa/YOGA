#pragma once
#include <t-infinity/GlobalToLocal.h>
#include <t-infinity/MeshInterface.h>
#include <map>

namespace inf {
inline std::map<int, int> localToLocalNodes(const MeshInterface& from, const MeshInterface& to) {
    auto g2l = inf::GlobalToLocal::buildNode(to);
    std::map<int, int> l2l;
    for (int i = 0; i < from.nodeCount(); i++) {
        auto global = from.globalNodeId(i);
        if (g2l.count(global) != 0) l2l[i] = g2l.at(global);
    }
    return l2l;
}

inline std::map<int, int> localToLocalCells(const MeshInterface& from, const MeshInterface& to) {
    auto g2l = inf::GlobalToLocal::buildCell(to);
    std::map<int, int> l2l;
    for (int i = 0; i < from.cellCount(); i++) {
        auto global = from.globalCellId(i);
        if (g2l.count(global) != 0) l2l[i] = g2l.at(global);
    }
    return l2l;
}
}
