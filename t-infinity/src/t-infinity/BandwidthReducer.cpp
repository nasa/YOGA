#include "BandwidthReducer.h"
#include "MeshConnectivity.h"
#include "ReorderMesh.h"
#include "ReverseCutthillMckee.h"

namespace inf {

std::shared_ptr<MeshInterface> BandwidthReducer::reoderNodes(std::shared_ptr<MeshInterface> mesh) {
    auto n2n = NodeToNode::build(*mesh);
    std::vector<bool> do_own(mesh->nodeCount());
    for (int n = 0; n < mesh->nodeCount(); n++) {
        do_own[n] = mesh->ownedNode(n);
    }
    auto old_to_new_ordering = ReverseCutthillMckee::calcNewIds(n2n, do_own);
    return MeshReorder::reorderMeshNodes(mesh, old_to_new_ordering);
}
}
