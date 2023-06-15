#pragma once
#include <MessagePasser/MessagePasser.h>
#include <memory>
#include <vector>
#include <t-infinity/MeshInterface.h>

namespace inf {
std::shared_ptr<inf::MeshInterface> addHaloNodes(MessagePasser mp,
                                                 const inf::MeshInterface& mesh_in);
std::shared_ptr<inf::MeshInterface> addHaloNodes(MessagePasser mp,
                                                 const inf::MeshInterface& mesh_in,
                                                 const std::set<int>& nodes_to_augment);
std::shared_ptr<inf::MeshInterface> addHaloCells(MessagePasser mp,
                                                 const inf::MeshInterface& mesh_in);
}
