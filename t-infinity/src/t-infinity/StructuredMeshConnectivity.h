#pragma once

#include <map>
#include <MessagePasser/MessagePasser.h>
#include "StructuredMesh.h"
#include "StructuredBlockFace.h"

namespace inf {
using AllBlockFaces = std::map<int, std::array<BlockFace, 6>>;
using BlockFaceConnectivity = std::map<BlockSide, FaceConnectivity>;
using MeshConnectivity = std::map<int, BlockFaceConnectivity>;

void matchFace(MeshConnectivity& mesh_connectivity,
               const AllBlockFaces& all_faces,
               const BlockFace& face);
MeshConnectivity buildMeshBlockConnectivity(const MessagePasser& mp, const StructuredMesh& mesh);
BlockFaceConnectivity buildBlockConnectivityFromLAURABoundaryConditions(
    const std::array<int, 6>& boundary_conditions);
MeshConnectivity buildMeshConnectivityFromLAURABoundaryConditions(const std::string& filename);
}
