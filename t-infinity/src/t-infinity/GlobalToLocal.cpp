#include "GlobalToLocal.h"
std::map<long, int> inf::GlobalToLocal::buildNode(const inf::MeshInterface& mesh) {
    std::map<long, int> g2l;
    for (int local = 0; local < mesh.nodeCount(); local++) {
        g2l[mesh.globalNodeId(local)] = local;
    }
    return g2l;
}
std::map<long, int> inf::GlobalToLocal::buildCell(const inf::MeshInterface& mesh) {
    std::map<long, int> g2l;
    for (int local = 0; local < mesh.cellCount(); local++) {
        g2l[mesh.globalCellId(local)] = local;
    }
    return g2l;
}
std::vector<long> inf::LocalToGlobal::buildCell(const inf::MeshInterface& mesh) {
    std::vector<long> l2g(mesh.cellCount());
    for (int c = 0; c < mesh.cellCount(); c++) {
        auto g = mesh.globalCellId(c);
        l2g[c] = g;
    }
    return l2g;
}
std::vector<long> inf::LocalToGlobal::buildNode(const inf::MeshInterface& mesh) {
    std::vector<long> l2g(mesh.nodeCount());
    for (int n = 0; n < mesh.nodeCount(); n++) {
        auto g = mesh.globalNodeId(n);
        l2g[n] = g;
    }
    return l2g;
}
