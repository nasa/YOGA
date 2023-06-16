#pragma once
#include <set>
#include <MessagePasser/MessagePasser.h>
#include <parfait/Point.h>
#include "MeshInterface.h"

namespace inf {
namespace MeshMirror {

    class MirroredMeshZipper {
      public:
        MirroredMeshZipper(const MessagePasser& mp,
                           const inf::MeshInterface& mesh,
                           const inf::MeshInterface& reflected_mesh,
                           const std::set<int>& nodes_to_remove_from_reflected_mesh);

        std::shared_ptr<inf::MeshInterface> getMesh() const;
        int previousNodeId(int id);

      private:
        std::vector<int> combined_local_node_id_to_original;
        std::shared_ptr<MeshInterface> zipped_mesh = nullptr;
    };

    Parfait::Point<double> getBoundaryPointWithTag(const MessagePasser& mp,
                                                   const MeshInterface& mesh,
                                                   int tag);
    double getDistanceFromMirrorSurfaceToPlane(const MessagePasser mp,
                                               const MeshInterface& mesh,
                                               const std::set<int>& tags,
                                               const Parfait::Point<double>& plane_normal);
    MirroredMeshZipper mirrorViaTags(const MessagePasser& mp,
                                     std::shared_ptr<MeshInterface> mesh,
                                     const std::set<int>& mirror_tags);
    MirroredMeshZipper mirrorViaTags(const MessagePasser& mp,
                                     std::shared_ptr<MeshInterface> mesh,
                                     const std::set<int>& mirror_tags,
                                     const Parfait::Point<double>& plane_normal,
                                     double plane_offset);
}
}
