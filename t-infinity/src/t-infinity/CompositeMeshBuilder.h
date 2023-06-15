#pragma once
#include <memory>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/TinfMesh.h>
#include <MessagePasser/MessagePasser.h>

namespace inf {

class CompositeMeshBuilder {
  public:
    static std::shared_ptr<inf::TinfMesh> createCompositeMesh(
        MessagePasser mp, std::vector<std::shared_ptr<inf::MeshInterface>> meshes);
    static std::vector<std::map<int, int>> calcMapOfNewSurfaceCellTags(
        std::vector<std::shared_ptr<inf::MeshInterface>> meshes, MessagePasser mp);
};
}
