#pragma once
#include <memory>
#include "TinfMesh.h"
#include "Communicator.h"
#include <t-infinity/MeshInterface.h>

namespace inf {
std::shared_ptr<TinfMesh> explodeMesh(MessagePasser mp,
                                      std::shared_ptr<MeshInterface> mesh,
                                      double scale = 0.8);
}