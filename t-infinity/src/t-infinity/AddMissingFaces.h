#pragma once
#include "TinfMesh.h"

namespace inf {
void assignOwnedSurfaceCellGlobalIds(const MessagePasser& mp, TinfMesh& mesh);
std::shared_ptr<TinfMesh> addMissingFaces(MessagePasser mp,
                                          const MeshInterface& m,
                                          const std::vector<std::vector<int>>& face_neighbors,
                                          int new_tag = 0);
std::shared_ptr<TinfMesh> addMissingFaces(MessagePasser mp,
                                          const MeshInterface& m,
                                          int new_tag = 0);
}
